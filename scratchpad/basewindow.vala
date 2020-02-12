using GLib;
using Gdk;
using Gtk;

public abstract class BaseWindow : Gtk.Window
{
	private const string[] authors = {
			"Daniel Borgmann <daniel.borgmann@gmail.com>", 
			null
	};

	private static Clipboard clipboard;
	
	protected DocumentView document_view;
	protected VBox document_box;
	protected ScrolledWindow scrolled_window;
	protected UIManager ui_manager;
	protected HPaned pane;
	
	private AboutDialog about_dialog;
	private HBox searchbar;
	private SearchEntry searchentry;
	private Statusbar statusbar;
	private Statusbar secondary_statusbar;
	private uint flash_timeout;
	private double hscroll;
	private double vscroll;

	construct {
		set_position (WindowPosition.CENTER);
		clipboard = Clipboard.@get (Atom.intern ("CLIPBOARD", false));
		setup_ui_manager ();
	}
	
	public static Clipboard get_clipboard () {
		return clipboard;
	}
	
	public virtual bool is_document (string uri) {
		return false;
	}
	
	protected bool save_scroll_position () {
		if (document_view == null) return false;	
		
		long hscroll = (long) scrolled_window.get_hadjustment ().get_value ();
		long vscroll = (long) scrolled_window.get_vadjustment ().get_value ();
		
		document_view.document.metadata.@set ("hscroll", hscroll.to_string ());
		document_view.document.metadata.@set ("vscroll", vscroll.to_string ());
		
		return false;
	}
	
	protected void restore_scroll_position () {
		hscroll = document_view.document.metadata.get_long ("hscroll");
		vscroll = document_view.document.metadata.get_long ("vscroll");
		
		GLib.Timeout.add (50, scroll_timeout);
	}
	
	private bool scroll_timeout () {
		if ( vscroll <= scrolled_window.get_vadjustment ().upper &&
		     hscroll <= scrolled_window.get_hadjustment ().upper ) {
			scrolled_window.get_vadjustment ().set_value (vscroll);
			scrolled_window.get_hadjustment ().set_value (hscroll);
			return false;
		} else {
			return true;
		}
	}
	
	protected void setup_ui_manager () {
		ui_manager = new CustomUIManager ();
		ui_manager.post_activate += on_action_activated;
		ui_manager.connect_proxy += on_connect_proxy;
		ui_manager.disconnect_proxy += on_disconnect_proxy;
		try {
			ui_manager.add_ui_from_file (SCRATCHPADDIR + "/scratchpad-ui.xml");
		} catch (GLib.Error e) {
		}
		add_accel_group (ui_manager.get_accel_group ());
	}
	
	protected void update_menu_state () {
		ToggleAction ta;
	
		ta = (ToggleAction)ui_manager.get_action ("/menubar/document/word-wrap");
		if ( document_view.document.metadata.@get ("word-wrap") == "true" ) {
			ta.set_active (true);
		} else if ( document_view.document.metadata.@get ("word-wrap") == "true" ) {
			ta.set_active (false);
		} else {
			// word wrap default is on for plaintext and off for source code
			if ( document_view.document.get_mimetype () == "text/plain" ) {
				ta.set_active (true);
			} else {
				ta.set_active (false);
			}
		}
		
		ui_manager.get_action ("/menubar/edit/undo").set_sensitive (document_view.document.can_undo);
		ui_manager.get_action ("/menubar/edit/redo").set_sensitive (document_view.document.can_redo);
	}
	
	protected virtual void connect_events () {
		delete_event += on_close;
		searchentry.key_press_event += on_searchentry_key_press;
		searchentry.focus_out_event += on_searchentry_focus_out;
		searchentry.search_activated += on_searchentry_search_activated;
		searchentry.search_closed += on_searchentry_closed;
		document_view.document.mark_set += on_document_mark_set;
		document_view.document.notify["can_undo"] += on_undo_redo_changed;
		document_view.document.notify["can_redo"] += on_undo_redo_changed;
		document_view.focus_out_event += (o, e) => {save_scroll_position ();};
	}
	
	protected virtual void populate_window () {
		/* placeholder for the actual document view */
		document_box = new VBox (false, 0);
		
		scrolled_window = new ScrolledWindow (null, null);
		scrolled_window.set_policy (PolicyType.AUTOMATIC, PolicyType.AUTOMATIC);
		scrolled_window.set_shadow_type (ShadowType.IN);
		
		/* searchbar */
		Label lbl = new Label ("Find:");
		searchentry = new SearchEntry (lbl);
		searchbar = new HBox (false, 0);
		searchbar.no_show_all = true;
		searchbar.set_spacing (6);
		searchbar.set_border_width (3);
		searchbar.pack_start (lbl, false, false, 0);
		searchbar.pack_start (searchentry, true, true, 0);
		
		/* document area */
		var document_area = new VBox (false, 0);
		document_area.pack_start (document_box, true, true, 0);
		document_area.pack_start (searchbar, false, false, 0);
		
		/* the side pane is used for project windows */
		pane = new HPaned ();
		pane.set_position (200);
		pane.add2 (document_area);
		
		/* statusbar */
		var statusbox = new HBox (false, 0);
		statusbar = new Statusbar ();
		secondary_statusbar = new Statusbar ();
		statusbar.set_has_resize_grip (false);
		secondary_statusbar.set_size_request (168, 0);
		statusbox.pack_start (statusbar, true, true, 0);
		statusbox.pack_start (secondary_statusbar, false, true, 0);
		
		/* main box */
		var mainbox = new VBox (false, 0);
		mainbox.pack_start (ui_manager.get_widget ("/menubar"), false, true, 0);
		mainbox.pack_start (pane, true, true, 0);
		mainbox.pack_start (statusbox, false, true, 0);
		
		add (mainbox);
	}
	
	private void flash_statusbar (string str)
	{
		flash_statusbar_clear ();
		
		statusbar.push (0, str);
		flash_timeout = GLib.Timeout.add (3000, flash_statusbar_timeout);
	}
	
	private void flash_statusbar_clear ()
	{
		statusbar.pop (0);
		if ( flash_timeout > 0 ) {
			GLib.Source.remove (flash_timeout);
			flash_timeout = 0;
		}
	}
	
	private bool flash_statusbar_timeout ()
	{
		statusbar.pop (0);
		return false;
	}
	
	private void show_about_dialog () {
		Pixbuf logo;
	
		try {
			logo = new Pixbuf.from_file (DATADIR + "/icons/hicolor/scalable/apps/scratchpad.svg");
		} catch (GLib.Error e) {
			return;
		}
		
		Gtk.show_about_dialog (this,
			"program-name", "Glasscat",
			"logo", logo,
			"authors", authors,
			"version", VERSION,
			"license", "Glasscat is free software; you can redistribute it and/or modify \nit under the terms of the GNU General Public License as published by \nthe Free Software Foundation; either version 2 of the License, or \n(at your option) any later version.\n\nGlasscat is distributed in the hope that it will be useful, \nbut WITHOUT ANY WARRANTY; without even the implied warranty of \nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License \nalong with Glasscat; if not, write to the Free Software Foundation, Inc., \n51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA",
			"comments", "Text editor with attitude",
			"copyright", "Copyright © 2006-2008 Daniel Borgmann",
			"website", "http://dborg.wordpress.com/scratchpad/",
			"website-label", "Visit the Glasscat Homepage"
		);
	}
		
	private bool close () {
		document_view.document.save ();
		save_scroll_position ();
		return false;
	}
	
	private void on_undo_redo_changed (GLib.Object o, GLib.ParamSpec s) {
		update_menu_state ();
	}
	
	private void on_document_mark_set (GLib.Object o, TextIter location, TextMark mark) {	
		if (secondary_statusbar == null) {
			// signal may be emitted while disposing
			return;
		}
	
		secondary_statusbar.pop (0);
		
		TextIter iter;
		document_view.document.get_iter_at_mark (out iter, document_view.document.get_insert ());
		
		int row = iter.get_line () + 1;
		int col = iter.get_line_offset () + 1;
		
		secondary_statusbar.push (0, "  Ln: " + row.to_string () + ", Col: " + col.to_string ());
	}
	
	private void on_menu_item_select (MenuItem proxy) {
		Action action = proxy.get_action ();
		return_if_fail (action != null);
		
		if (action.tooltip != null) {
			statusbar.push (statusbar.get_context_id ("tooltip"), action.tooltip);
		}
	}
	
	private void on_menu_item_deselect (MenuItem proxy) {
		statusbar.pop (statusbar.get_context_id ("tooltip"));
	}
	
	private void on_connect_proxy (UIManager manager, Action action, Widget proxy) {
		((MenuItem) proxy).select += on_menu_item_select;
		((MenuItem) proxy).deselect += on_menu_item_deselect;
	}
	
	private void on_disconnect_proxy (UIManager manager, Action action, Widget proxy) {
		((MenuItem) proxy).select -= on_menu_item_select;
		((MenuItem) proxy).deselect -= on_menu_item_deselect;
	}
	
	private void on_searchentry_search_activated (GLib.Object o, SearchEventArgs args) {
		if ( args.mode == SearchMode.GO_TO_LINE ) {
			document_view.go_to_line (args.search_text.to_int ());
			return;
		}
		
		if ( args.mode == SearchMode.REPLACE ) {
			document_view.replace (args.search_text);
			return;
		}
	
		TextIter i, j, start;
		document_view.document.get_selection_bounds (out i, out j);
		start = (args.next_result && !args.backwards) ? j : i;
		
		// search case insensitive if string is all lower case
		bool match_case = (args.search_text != args.search_text.casefold ()) ? true : false;
		
		if ( !document_view.find (args.search_text, start, match_case, args.backwards) ) {
			flash_statusbar ("No search result for \"" + args.search_text + "\"");
		} else {
			flash_statusbar_clear ();
		}
	}
	
	private bool on_searchentry_key_press (GLib.Object o, EventKey key) {
		// unfocus and hide the searchbar when escape is pressed
		if ( Gdk.keyval_name(key.keyval) == "Escape" ) {
			document_view.grab_focus ();
		}
		return false;
	}
	
	private bool on_searchentry_focus_out (SearchEntry o, EventFocus e) {
		searchbar.hide ();
		return false;
	}
	
	private void on_searchentry_closed (GLib.Object o) {
		searchbar.hide ();
	}
	
	private void on_action_activated (GLib.Object o, Action action) {
		ToggleAction ta;
	
		// TODO: replace with switch statement once it works with strings
		/* File menu */
		if ( action.name == "open-folder" ) {
			try {
				Process.spawn_command_line_async ("gnome-open " + document_view.document.get_folder ());
			} catch (GLib.Error e) {
			}
		} else if ( action.name == "close" ) {
			destroy ();
		/* Edit menu */
		} else if ( action.name == "undo" ) {
			if (document_view.document.can_undo) document_view.document.undo ();
			update_menu_state ();
		} else if ( action.name == "redo" ) {
			if (document_view.document.can_redo) document_view.document.redo ();
			update_menu_state ();
		} else if ( action.name == "cut" ) {
			document_view.block_cut_clipboard ();
		} else if ( action.name == "copy" ) {
			document_view.block_copy_clipboard ();
		} else if ( action.name == "paste" ) {
			document_view.block_paste_clipboard ();
		} else if ( action.name == "block-cut" ) {
			document_view.block_cut_clipboard (true);
		} else if ( action.name == "block-copy" ) {
			document_view.block_copy_clipboard (true);
		} else if ( action.name == "find" ) {
			searchentry.set_mode (SearchMode.TEXT);
			searchbar.no_show_all = false;
			searchbar.show_all ();
			searchentry.grab_focus ();
		} else if (action.name == "replace" ) {
			searchentry.set_mode (SearchMode.REPLACE);
			searchbar.no_show_all = false;
			searchbar.show_all ();
			searchentry.grab_focus ();
		} else if (action.name == "goto-line" ) {
			searchentry.set_maxlines (document_view.document.get_line_count ());
			searchentry.set_mode (SearchMode.SILENT);
			searchentry.set_text (document_view.get_current_line ().to_string ());
			searchentry.set_mode (SearchMode.GO_TO_LINE);
			searchbar.no_show_all = false;
			searchbar.show_all ();
			searchentry.grab_focus ();
		} else if (action.name == "select-all" ) {
			document_view.select_all ();
		} else if (action.name == "new-line" ) {
			document_view.insert_new_line ();
		/* Tags submenu */
		} else if (action.name == "tag-add" ) {
			document_view.tag_selection ();
		} else if (action.name == "tag-remove" ) {
			document_view.remove_tags ();
		} else if (action.name == "tag-search" ) {
			document_view.tag_search_results (searchentry.get_last_search ());
		} else if (action.name == "tag-add-and-search" ) {
			document_view.tag_and_find_next ();
		/* View menu */
		} else if (action.name == "line-numbers" ) {
			document_view.show_line_numbers = !document_view.show_line_numbers;
			Settings.set_show_line_numbers (document_view.show_line_numbers);
		} else if (action.name == "margin" ) {
			document_view.show_right_margin = !document_view.show_right_margin;
			Settings.set_show_right_margin (document_view.show_right_margin);
		/* Document menu */
		} else if (action.name == "word-wrap" ) {
			document_view.wrap_mode = (((ToggleAction)action).active) ? WrapMode.WORD : WrapMode.NONE;
		/* Help menu */
		} else if ( action.name == "about" ) {
			show_about_dialog ();
		}
	}
	
	private bool on_close (GLib.Object o) {
		return close ();
	}
}
