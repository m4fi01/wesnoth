/*
   Copyright (C) 2014 by Ignacio R. Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "gui/dialogs/wml_error.hpp"

#include "addon/info.hpp"
#include "addon/manager.hpp"
#include "filesystem.hpp"
#include "gui/auxiliary/find_widget.tpp"
#include "gui/widgets/control.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "serialization/string_utils.hpp"

namespace
{
inline bool is_dir_separator(char c)
{
#ifdef _WIN32
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

void strip_trailing_dir_separators(std::string& str)
{
	while(is_dir_separator(str[str.size() - 1])) {
		str.erase(str.size() - 1);
	}
}

std::string format_file_list(const std::vector<std::string>& files_original)
{
	if(files_original.empty()) {
		return "";
	}

	const std::string& addons_path = get_addon_campaigns_dir();
	std::vector<std::string> files(files_original);

	BOOST_FOREACH(std::string& file, files)
	{
		std::string base;
		std::string filename = file_name(file);
		std::string parent_path;

		const bool is_main_cfg = filename == "_main.cfg";

		if(is_main_cfg) {
			parent_path = directory_name(file) + "/..";
		} else {
			parent_path = directory_name(file);
		}

		// Only proceed to pretty-format the filename if it's from the add-ons
		// directory.
		if(normalize_path(parent_path) != normalize_path(addons_path)) {
			continue;
		}

		if(is_main_cfg) {
			base = directory_name(file);
			// HACK: fool file_name() into giving us the parent directory name
			//       alone by making base seem not like a directory path,
			//       otherwise it returns an empty string.
			strip_trailing_dir_separators(base);
			base = file_name(base);
		} else {
			base = filename;
		}

		if(base.empty()) {
			// We did something wrong. In the interest of not messing up the
			// report, leave the original filename intact.
			continue;
		}

		//
		// Display the name as an add-on name instead of a filename.
		//

		if(!is_main_cfg) {
			// Remove the file extension first.
			static const std::string wml_suffix = ".cfg";

			if(base.size() > wml_suffix.size()) {
				const size_t suffix_pos = base.size() - wml_suffix.size();
				if(base.substr(suffix_pos) == wml_suffix) {
					base.erase(suffix_pos);
				}
			}
		}

		if(have_addon_install_info(base)) {
			// _info.cfg may have the add-on's title starting with 1.11.7,
			// if the add-on was downloaded using the revised _info.cfg writer.
			config cfg;
			get_addon_install_info(base, cfg);

			const config& info_cfg = cfg.child("info");

			if(info_cfg) {
				file = info_cfg["title"].str();
				continue;
			}
		}

		// Fallback to using a synthetic title with underscores replaced with
		// whitespace.
		file = make_addon_title(base);
	}

	if(files.size() == 1) {
		return files.front();
	}

	return utils::bullet_list(files);
}

}

namespace gui2
{

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_wml_error
 *
 * == WML error ==
 *
 * Dialog used to report WML parser or preprocessor errors.
 *
 * @begin{table}{dialog_widgets}
 *
 * summary & & control & m &
 *         Label used for displaying a brief summary of the error(s). $
 *
 * files & & control & m &
 *         Label used to display the list of affected add-ons or files, if
 *         applicable. It is hidden otherwise. It is recommended to place it
 *         after the summary label. $
 *
 * details & & control & m &
 *         Full report of the parser or preprocessor error(s) found. $
 *
 * @end{table}
 */

REGISTER_DIALOG(wml_error)

twml_error::twml_error(const std::string& summary,
					   const std::vector<std::string>& files,
					   const std::string& details)
	: have_files_(!files.empty())
{
	register_label("summary", true, summary);
	register_label("files", true, format_file_list(files));
	register_label("details", true, details);
}

void twml_error::pre_show(CVideo& /*video*/, twindow& window)
{
    if(!have_files_) {
		tcontrol& filelist = find_widget<tcontrol>(&window, "files", false);
		filelist.set_visible(tcontrol::tvisible::invisible);
	}
}

} // end namespace gui2
