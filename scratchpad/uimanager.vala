using GLib;
using Gtk;

public class CustomUIManager : UIManager
{
	private ActionGroup action_group;
	private ActionGroup document_action_group;

	construct {
		ToggleAction ta;
	
		action_group = new ActionGroup ("general");
		document_action_group = new ActionGroup ("document");
		
		action_group.add_action (new Action ("file", "_File", null, null));
		action_group.add_action (new Action ("edit", "_Edit", null, null));
		document_action_group.add_action (new Action ("tags", "_Tags", null, null));
		action_group.add_action (new Action ("view", "_View", null, null));
		action_group.add_action (new Action ("document", "_Document", null, null));
		action_group.add_action (new Action ("help", "_Help", null, null));
		
		/* File menu */
		action_group.add_action_with_accel (new Action ("close", "_Close", "Close this document", null), "<ctrl>w");
		action_group.add_action_with_accel (new Action ("open-folder", "_Open Folder", "Open the folder containing this document", null), "<ctrl>o");
		
		/* Edit menu */
		document_action_group.add_action_with_accel (new Action ("undo", "_Undo", "Undo the last action", null), "<ctrl>z");
		document_action_group.add_action_with_accel (new Action ("redo", "_Redo", "Redo the last action", null), "<shift><ctrl>z");
		document_action_group.add_action_with_accel (new Action ("cut", "Cu_t", "Cut the selection or current line", null), "<ctrl>x");
		document_action_group.add_action_with_accel (new Action ("copy", "_Copy", "Copy the selection or current line", null), "<ctrl>c");
		document_action_group.add_action_with_accel (new Action ("paste", "_Paste", "Paste the clipboard", null), "<ctrl>v");
		document_action_group.add_action_with_accel (new Action ("block-cut", "_Block Cut", "Cut the selected block", null), "<shift><ctrl>x");
		document_action_group.add_action_with_accel (new Action ("block-copy", "Bl_ock Copy", "Copy the selected block", null), "<shift><ctrl>c");
		document_action_group.add_action_with_accel (new Action ("new-line", "Insert _New Line", "Insert a new line below the active line", null), "<ctrl>Return");
		document_action_group.add_action_with_accel (new Action ("find", "_Find...", "Search for text", null), "<ctrl>f");
		document_action_group.add_action_with_accel (new Action ("replace","_Replace...", "Replace all tags", null), "<ctrl>r");
		document_action_group.add_action_with_accel (new Action ("goto-line", "Go to _Line...", "Go to a specific line", null), "<ctrl>l");
		document_action_group.add_action_with_accel (new Action ("select-all", "Select _All", "Select the entire document", null), "<ctrl>a");
		
		/* Tags submenu */
		document_action_group.add_action_with_accel (new Action ("tag-add", "_Tag Selection", "Tag the current selection", null), "<ctrl>t");
		document_action_group.add_action_with_accel (new Action ("tag-add-and-search", "Tag and _Find Next", "Tag the current selection and find a similar one", null), "<ctrl>g");
		document_action_group.add_action_with_accel (new Action ("tag-remove", "_Remove Tags", "Remove all tags under current selection or entire document", null), "<shift><ctrl>t");
		document_action_group.add_action_with_accel (new Action ("tag-search", "Tag _Search Results", "Tag all matches from last search", null), "<shift><ctrl>f");
		
		/* View menu */
		ta = new ToggleAction ("line-numbers", "_Line Numbers", "Toggle display of line numbers", null);
		ta.active = Settings.get_show_line_numbers ();
		action_group.add_action (ta);
		ta = new ToggleAction ("margin", "Right _Margin", "Toggle display of 80 character margin", null);
		ta.active = Settings.get_show_right_margin ();
		action_group.add_action (ta);
		
		/* Document menu */
		ta = new ToggleAction ("word-wrap", "_Word Wrap", "Enable or disable text wrapping", null);
		ta.active = false;
		document_action_group.add_action (ta);
		
		/* Help menu */
		action_group.add_action (new Action ("about", "_About", "About this application", null));
		
		insert_action_group (action_group, 0);
		insert_action_group (document_action_group, 0);
	}
}
