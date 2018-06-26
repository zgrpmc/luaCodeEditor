/*

Decoda
Copyright (C) 2007-2013 Unknown Worlds Entertainment, Inc. 

This file is part of Decoda.

Decoda is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Decoda is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Decoda.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef PROJECT_EXPLORER_WINDOW_H
#define PROJECT_EXPLORER_WINDOW_H

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/thread.h>
#include <vector>
#include <hash_set>

#include "Project.h"

//
// Forward declarations.
//

class SearchTextCtrl;
class Symbol;
class ProjectFileInfoCtrl;
class ProjectFilterPopup;

/**
 * Tree view that displays the files in a project.
 */
class ProjectExplorerWindow : public wxPanel
{

public:
	void openSelectedFile();
	void focusSearch();
    /**
     * Stores data for a node in the tree.
     */
    struct ItemData : public wxTreeItemData
    {
        Project::File*      file;
        Symbol*             symbol;
    };

    enum FilterFlag
    {
        FilterFlag_Temporary    = 1 << 0,
        FilterFlag_Unversioned  = 1 << 1,
        FilterFlag_CheckedIn    = 1 << 2,
        FilterFlag_CheckedOut   = 1 << 3,
    };

    /**
     * Constructor.
     */
    ProjectExplorerWindow(wxWindow* parent, wxWindowID winid);

    /**
     * Destructor.
     */
    virtual ~ProjectExplorerWindow();

    /**
     * Sets the keyboard focus to the filter text box.
     */
    void SetFocusToFilter();

    /**
     * Called when the user changes the text in the filter box.
     */
    void OnFilterTextChanged(wxCommandEvent& event);

    /**
     * Called when the user expands an item in the tree.
     */
    void OnTreeItemExpanding(wxTreeEvent& event);

    /**
     * Called when the user collapses an item in the tree.
     */
    void OnTreeItemCollapsing(wxTreeEvent& event);

    /**
     * Called when the user double clicks on an item in the tree.
     */
    void OnTreeItemActivated(wxTreeEvent& event);

    /**
     * Called when the user right clicks on an item in the tree.
     */
    void OnTreeItemContextMenu(wxTreeEvent& event);

    /**
     * Called when a new item is selected in the tree.
     */
    void OnTreeItemSelectionChanged(wxTreeEvent& event);

    /**
     * Called when the user displays the context menu.
     */
    void OnMenu(wxTreeEvent& event);

    /**
     * Called when the user clicks on the filter button.
     */
    void OnFilterButton(wxCommandEvent& event);

    /**
     * Rebuilds the entire tree for the project.
     */
    void SetProject(Project* project);

    /**
     * Gets the file that corresponds to an item in the tree.
     */
    ItemData* GetDataForItem(wxTreeItemId id) const;

    /**
     * Returns the currently selected item in the tree view. Note that multiple items
     * could be selected and this only returns the first one.
     */
    wxTreeItemId GetSelection() const;

    /**
     * Adds references for the file into the explorer view. This should be
     * called when a file is added to the project.
     */
    void InsertFile(Project::File* file);

    /**
     * Removes all references to a file from the explorer view. This should
     * be called when a file is deleted from the project.
     */
    void RemoveFile(Project::File* file);

    /**
     * Removes all references to the specified files from the explorer view. This
     * should be called when a file is deleted from the project.
     */
    void RemoveFiles(const std::vector<Project::File*>& files);

    /**
     * Sets the menu displays when the user right clicks on a file. Ownership
     * retains with the caller.
     */
    void SetFileContextMenu(wxMenu* menu);

    /**
     * Gets a list of all of the files that are currently selected in the tree.
     * Non-files that are selected are ignored.
     */
    void GetSelectedFiles(std::vector<Project::File*>& selectedFiles) const;

    /**
     * Updates the images showing the file status.
     */
    void UpdateFileImages();

    /**
     * Updates the window for the specified file. This should be called when the
     * symbols for the file change.
     */
    void UpdateFile(Project::File* file);

    /**
     * Sets the flags that determine what types of files are displayed in the project
     * explorer window. These should be a combination of the FilterFlag enum values.
     */
    void SetFilterFlags(unsigned int filterFlags);

    /**
     * Returns the flags that determine what types of files are displayed in the project
     * explorer window. These are a combination of the FilterFlag enum values.
     */
    unsigned int GetFilterFlags() const;
    
    enum ID
    {
        ID_Filter       = 1,
        ID_FilterButton = 2,
    };

    DECLARE_EVENT_TABLE()

private:

    /**
     * Returns true if the string matches the filter.
     */
    bool MatchesFilter(const wxString& string, const wxString& filter) const;

    /**
     * Adds a new file to the tree.
     */
    void AddFile(wxTreeItemId parent, Project::File* file);

    /**
     * Adds a new symbol to the tree.
     */
    void AddSymbol(wxTreeItemId parent, Project::File* file, Symbol* symbol);

    /**
     * Removes any child nodes of the specified node that reference the file.
     */
    void RemoveFileSymbols(wxTreeItemId node, Project::File* file);

    /**
     * Removes any child nodes of the specified node that reference the file.
     */
    void RemoveFileSymbols(wxTreeItemId node, const stdext::hash_set<Project::File*>& file);

    /**
     * Rebuilds the entire list in the tree control. This should be done when
     * the filter changes.
     */
    void Rebuild();

    /**
     * Adds the items for the file that match the filter into the tree control.
     */
    void RebuildForFile(Project::File* file);

    /**
     * Returns the image that should be used to display the file.
     */
    int GetImageForFile(const Project::File* file) const;

    /**
     * Adds a new symbol definition from the file.
     */
    void AddSymbol(Project::File* file, const wxString& module, const wxString& name, unsigned int lineNumber);

    /**
     * Updates the image draw on the filter button based on the currently
     * selected filter flags.
     */
    void UpdateFilterButtonImage();

private:
    
    /**
     * These correspond to the order of the images in explorer.xpm.
     */
    enum Image
    {
        Image_Module                = 0,
        Image_File                  = 1,
        Image_Function              = 2,
        Image_FileTemp              = 3,
        Image_FileCheckedIn         = 4,
        Image_FileTempCheckedIn     = 5,
        Image_FileCheckedOut        = 6,
        Image_FileTempCheckedOut    = 7,
    };

    Project*                    m_project;

    SearchTextCtrl*             m_searchBox;
    wxString                    m_filter;
    bool                        m_filterMatchAnywhere;

    wxTreeItemId                m_root;
    wxTreeCtrl*                 m_tree;

    wxImageList*                m_filterImageList;
    wxBitmapButton*             m_filterButton;
    ProjectFilterPopup*         m_filterPopup;
    unsigned int                m_filterFlags;

    ProjectFileInfoCtrl*        m_infoBox;

    wxTreeItemId                m_stopExpansion;

    wxMenu*                     m_contextMenu;


};

#endif
