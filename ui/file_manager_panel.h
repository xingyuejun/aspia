//
// PROJECT:         Aspia Remote Desktop
// FILE:            ui/file_manager_panel.h
// LICENSE:         See top-level directory
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#ifndef _ASPIA_UI__FILE_MANAGER_PANEL_H
#define _ASPIA_UI__FILE_MANAGER_PANEL_H

#include "ui/base/child_window.h"
#include "ui/base/comboboxex.h"
#include "ui/base/listview.h"
#include "ui/base/imagelist.h"
#include "proto/file_transfer_session.pb.h"

namespace aspia {

class UiFileManagerPanel : public UiChildWindow
{
public:
    UiFileManagerPanel() = default;
    virtual ~UiFileManagerPanel() = default;

    enum class PanelType { UNKNOWN, LOCAL, REMOTE };

    class Delegate
    {
    public:
        virtual void OnDriveListRequest(PanelType panel_type) = 0;
        virtual void OnDirectoryListRequest(PanelType _panel_type, const std::string& path) = 0;
        virtual void OnCreateDirectoryRequest(PanelType panel_type, const std::string& path) = 0;
        virtual void OnRenameRequest(PanelType panel_type, const std::string& old_path, const std::string& new_path) = 0;
        virtual void OnRemoveRequest(PanelType panel_type, const std::string& path) = 0;
    };

    bool CreatePanel(HWND parent, PanelType panel_type, Delegate* delegate);

    void ReadDriveList(std::unique_ptr<proto::DriveList> drive_list);
    void ReadDirectoryList(std::unique_ptr<proto::DirectoryList> directory_list);

private:
    void OnCreate();
    void OnDestroy();
    void OnSize(int width, int height);
    void OnDrawItem(LPDRAWITEMSTRUCT dis);
    void OnGetDispInfo(LPNMHDR phdr);
    void OnDriveChange();
    void OnAddressChange();
    void OnFolderUp();
    void OnFolderCreate();
    void OnRefresh();
    void OnRemove();
    void OnMoveToComputer();
    void OnEndLabelEdit(LPNMLVDISPINFOW disp_info);
    int GetKnownDriveIndex(const std::string& path);

    // ChildWindow implementation.
    bool OnMessage(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* result) override;

    PanelType panel_type_ = PanelType::UNKNOWN;
    Delegate* delegate_ = nullptr;

    UiWindow title_window_;

    std::unique_ptr<proto::DirectoryList> directory_list_;
    UiListView list_window_;
    UiImageList list_imagelist_;

    UiWindow toolbar_window_;
    UiImageList toolbar_imagelist_;

    std::unique_ptr<proto::DriveList> drive_list_;
    UiComboBoxEx drives_combo_;
    UiImageList drives_imagelist_;

    UiWindow status_window_;

    DISALLOW_COPY_AND_ASSIGN(UiFileManagerPanel);
};

} // namespace aspia

#endif // _ASPIA_UI__FILE_MANAGER_PANEL_H