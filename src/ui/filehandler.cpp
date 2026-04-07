#include "ui/filehandler.h"
#include "utils/SimpleFileDialog.h"

void FileHandler::openBrowser()
{
    if (!is_window_open)
    {
        SimpleFileDialog::Instance()->OpenDialog("ChooseFileKey", "Select Data File", ".txt,.csv", ".");
        is_window_open = true;
    }
}

void FileHandler::updateAndDisplay()
{
    if (SimpleFileDialog::Instance()->Display("ChooseFileKey")) {
        if (SimpleFileDialog::Instance()->IsOk()) {
            std::string _temp_path = SimpleFileDialog::Instance()->GetFilePathName();
            selected_path = _temp_path;
        }
        SimpleFileDialog::Instance()->Close();
        is_window_open = false;
    }
}

bool FileHandler::hasSelected() const {
    return !selected_path.empty();
}
