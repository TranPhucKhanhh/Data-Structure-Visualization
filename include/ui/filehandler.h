#pragma once

#include<string>

struct FileHandler
{
    std::string selected_path;
    bool is_window_open;

    void openBrowser();
    void updateAndDisplay();

    bool hasSelected() const;
};
