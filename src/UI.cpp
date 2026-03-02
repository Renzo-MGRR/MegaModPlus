#include "UI.h"
#include <Windows.h>
#include <IniReader.h>
#include "Shlwapi.h"
#include <stdlib.h>
#include <ShlObj.h>
#include <shobjidl.h>
#include <atlbase.h>
#include <filesystem>
#include <toml.hpp>
#include <bit7z\bitarchivereader.hpp>
#include <bit7z\bitfileextractor.hpp>
std::vector<std::wstring> getInDirectoryW(const std::wstring& directory, bool getFolder)
{
    std::vector<std::wstring> files;
    std::wstring searchPath = directory + L"\\*";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return files;

    do {
        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (isDir == getFolder &&
            wcscmp(fd.cFileName, L".") != 0 &&
            wcscmp(fd.cFileName, L"..") != 0)
        {
            files.push_back(fd.cFileName);
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    return files;
}
LPCSTR UI::lpWindowName = "MegaMod+";
CIniReader managerSettings("megamodplus.ini");
ImVec2 UI::vWindowSize = {
	managerSettings.ReadFloat("Settings", "WindowXSize", 960),
	managerSettings.ReadFloat("Settings", "WindowYSize", 540)
};
ImGuiWindowFlags UI::WindowFlags = 0;
bool FolderPick = false;
bool DoOnce = true;
bool UI::bDraw = true;
void UI::Active()
{
	bDraw = true;
}

bool UI::isActive()
{
	return bDraw == true;
}
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
std::string extractBetweenDelimiters(const std::string& str, const std::string& start_delim, const std::string& end_delim) {
    size_t first_delim_pos = str.find(start_delim);

    if (first_delim_pos == std::string::npos) {
        return "";
    }
    size_t start_pos = first_delim_pos + start_delim.length();
    size_t second_delim_pos = str.find(end_delim, start_pos);
    if (second_delim_pos == std::string::npos) {
        return "";
    }
    size_t length = second_delim_pos - start_pos;
    return str.substr(start_pos, length);
}

std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
char CurrentFolder[MAX_PATH];
std::string StrCurrentDirectory = "";
std::string TomlLocation = "";
std::wstring modsDir = L"";
bool ModLoaderEnabled = false;
bool ConsoleEnabled = false;
void UI::Draw()
{
    namespace fs = std::filesystem;
    if (DoOnce)
    {
        GetCurrentDirectoryA(MAX_PATH, CurrentFolder);
        StrCurrentDirectory = CurrentFolder;
        auto TheToml = toml::parse_file(StrCurrentDirectory + "\\config.toml");
        if (auto ModLoaderEnabledB = TheToml["enabled"].as_boolean())
        {
            ModLoaderEnabled = &ModLoaderEnabledB;
        }
        if (auto ConsoleEnabledB = TheToml["console"].as_boolean())
        {
            ConsoleEnabled = &ConsoleEnabledB;
        }
        std::string_view StrViewModsDir = TheToml["mods"].value_or("");
        modsDir = utf8_to_wstring(StrCurrentDirectory) + L"\\" + utf8_to_wstring(std::string(StrViewModsDir));
        if (!PathFileExistsW(modsDir.c_str()))
        {
            fs::create_directory(fs::path(modsDir));
        }
        DoOnce = false;
    }
	if (isActive())
	{
		ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		if (ImGui::Begin(lpWindowName, &bDraw, WindowFlags))
		{
            ImVec2 currentSize = ImGui::GetWindowSize();
            if (vWindowSize.x != currentSize.x)
            {
                managerSettings.WriteFloat("Settings", "WindowXSize", currentSize.x);
            }
            if (vWindowSize.y != currentSize.y)
            {
                managerSettings.WriteFloat("Settings", "WindowYSize", currentSize.y);
            }
            if (ImGui::BeginTabBar("NOTITLE", ImGuiTabBarFlags_NoTooltip))
            {
                if (ImGui::BeginTabItem("Mods"))
                {
                    ImGui::Text("Mods Dir: %s", wstring_to_utf8(modsDir).c_str());
                    ImGui::Checkbox("Enable the mod loader", &ModLoaderEnabled);
                    ImGui::Checkbox("Enable debug console", &ConsoleEnabled);
                    if (ImGui::Button("Add Mods"))
                    {   
                        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
                        if (SUCCEEDED(hr))
                        {
                            CComPtr<IFileOpenDialog> pFileDlg;
                            hr = pFileDlg.CoCreateInstance(CLSID_FileOpenDialog);

                            if (SUCCEEDED(hr))
                            {
                                DWORD dwFlags;
                                pFileDlg->GetOptions(&dwFlags);
                                pFileDlg->SetOptions(dwFlags | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM);

                                COMDLG_FILTERSPEC fileTypes[] = { { L"DIVA Mod Files", L"*.zip" } };
                                pFileDlg->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);

                                hr = pFileDlg->Show(NULL);
                                if (SUCCEEDED(hr))
                                {
                                    CComPtr<IShellItemArray> pResults;
                                    hr = pFileDlg->GetResults(&pResults);

                                    if (SUCCEEDED(hr))
                                    {
                                        DWORD dwCount = 0;
                                        pResults->GetCount(&dwCount);

                                        for (DWORD i = 0; i < dwCount; i++)
                                        {
                                            CComPtr<IShellItem> pItem;
                                            pResults->GetItemAt(i, &pItem);

                                            LPWSTR pszFilePath = NULL;
                                            pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                                            if (pszFilePath)
                                            {
                                                std::wstring filename = fs::path(pszFilePath).stem().wstring();
                                                std::wstring ModPackDir = fs::path(pszFilePath).parent_path().wstring() + L"\\" + filename;
                                                bit7z::Bit7zLibrary lib{ "7z.dll" };
                                                if (!PathFileExistsW(ModPackDir.c_str()))
                                                {
                                                    fs::create_directory(fs::path(ModPackDir));
                                                    bit7z::BitFileExtractor extractor{ lib, bit7z::BitFormat::Zip };
                                                    try {
                                                        extractor.extract(wstring_to_utf8(pszFilePath), wstring_to_utf8(ModPackDir));
                                                    }
                                                    catch (const::bit7z::BitException ex)
                                                    {

                                                    }
                                                }
                                                if (PathFileExistsW(ModPackDir.c_str()))
                                                {
                                                    fs::copy(fs::path(ModPackDir), fs::path(modsDir + L"\\" + filename), fs::copy_options::recursive);
                                                }
                                                CoTaskMemFree(pszFilePath);
                                            }
                                        }
                                        //UpdateModsList = true;
                                    }
                                }
                            }
                            CoUninitialize();
                        }
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Settings"))
                {
                    if (ImGui::Button("Locate the game's exe..."))
                    {

                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
		}
	}

	#ifdef _WINDLL
	if (GetAsyncKeyState(VK_INSERT) & 1)
		bDraw = !bDraw;
	#endif
}
