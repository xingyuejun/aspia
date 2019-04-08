//
// Aspia Project
// Copyright (C) 2019 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "desktop/screen_capturer_dfmirage.h"

#include "base/logging.h"
#include "desktop/dfmirage_helper.h"
#include "desktop/desktop_frame_aligned.h"
#include "desktop/win/screen_capture_utils.h"

namespace desktop {

ScreenCapturerDFMirage::ScreenCapturerDFMirage() = default;
ScreenCapturerDFMirage::~ScreenCapturerDFMirage() = default;

bool ScreenCapturerDFMirage::isSupported()
{
    if (!helper_)
    {
        // If DFMirageHelper is not created, then create it.
        helper_ = DFMirageHelper::create(ScreenCaptureUtils::fullScreenRect());
    }

    return helper_ != nullptr;
}

int ScreenCapturerDFMirage::screenCount()
{
    return ScreenCaptureUtils::screenCount();
}

bool ScreenCapturerDFMirage::screenList(ScreenList* screens)
{
    return ScreenCaptureUtils::screenList(screens);
}

bool ScreenCapturerDFMirage::selectScreen(ScreenId screen_id)
{
    if (!ScreenCaptureUtils::isScreenValid(screen_id, &current_device_key_))
        return false;

    reset();

    current_screen_id_ = screen_id;
    return true;
}

const Frame* ScreenCapturerDFMirage::captureFrame()
{
    if (!prepareCaptureResources())
        return nullptr;

    Region* updated_region = frame_->updatedRegion();

    // Clearing the region from previous changes.
    updated_region->clear();

    // Update the list of changed areas.
    helper_->addUpdatedRects(updated_region);

    // Exclude a region that should not be captured.
    updated_region->subtract(exclude_region_);

    // Copy the image of the modified areas into the frame.
    helper_->copyRegion(frame_.get(), *updated_region);

    frame_->setTopLeft(helper_->screenRect().topLeft());
    return frame_.get();
}

void ScreenCapturerDFMirage::reset()
{
    helper_.reset();
    frame_.reset();
}

void ScreenCapturerDFMirage::updateExcludeRegion()
{
    exclude_region_.clear();

    if (current_screen_id_ != kFullDesktopScreenId)
        return;

    exclude_region_.addRect(desktop_rect_.moved(0, 0));

    ScreenList screen_list;
    if (!ScreenCaptureUtils::screenList(&screen_list))
        return;

    for (const auto& screen : screen_list)
    {
        QString device_key;

        if (ScreenCaptureUtils::isScreenValid(screen.id, &device_key))
        {
            Rect screen_rect = ScreenCaptureUtils::screenRect(screen.id, device_key);

            int x = std::abs(screen_rect.x() - desktop_rect_.x());
            int y = std::abs(screen_rect.y() - desktop_rect_.y());
            int w = screen_rect.width();
            int h = screen_rect.height();

            exclude_region_.subtract(Rect::makeXYWH(x, y, w, h));
        }
    }
}

bool ScreenCapturerDFMirage::prepareCaptureResources()
{
    Rect desktop_rect = ScreenCaptureUtils::fullScreenRect();
    if (desktop_rect.isEmpty())
    {
        LOG(LS_WARNING) << "Failed to get desktop rect";
        return false;
    }

    if (desktop_rect != desktop_rect_)
    {
        desktop_rect_ = desktop_rect;
        reset();
    }

    Rect screen_rect = ScreenCaptureUtils::screenRect(current_screen_id_, current_device_key_);
    if (screen_rect.isEmpty())
    {
        LOG(LS_WARNING) << "Failed to get screen rect";
        return false;
    }

    if (helper_ && helper_->screenRect() != screen_rect)
        reset();

    if (frame_ && frame_->size() != screen_rect.size())
        reset();

    if (!helper_)
    {
        helper_ = DFMirageHelper::create(screen_rect);
        if (!helper_)
        {
            LOG(LS_WARNING) << "Failed to create DFMirage helper";
            return false;
        }

        updateExcludeRegion();
    }

    if (!frame_)
    {
        frame_ = FrameAligned::create(screen_rect.size(), PixelFormat::ARGB(), 32);
        if (!frame_)
        {
            LOG(LS_WARNING) << "Failed to create frame";
            return false;
        }
    }

    return true;
}

} // namespace desktop
