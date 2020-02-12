using GLib;
using Gtk;
using Pango;

public class Tag : GLib.Object
{
	public TextMark begin;
	public TextMark end;
}

public class DocumentView : SourceView
{
	private Document _document;
	public Document document { 
		get {
			return _document;
		}
		set {
			_document = value;
			set_buffer (_document);
			
			TextTag tag = new TextTag ("tag");
			tag.background = "yellow";
			document.get_tag_table ().add (tag);
			
			restore_state ();
		}
	}

	private SList<Tag> tag_list;
	private bool modified;

	public DocumentView () {}
	public DocumentView.with_document (Document document) {
		this.document = document;
	}

	construct {
		modify_font (FontDescription.from_string (SystemSettings.get_font ()));
		//GConf.Client.get_default().notify_add ("/desktop/gnome/interface/monospace_font_name", on_font_changed);
	
		this.tab_width = 8;
		this.insert_spaces_instead_of_tabs = false;
		this.indent_on_tab = true;
		this.highlight_current_line = true;
		this.auto_indent = true;
		this.draw_spaces = SourceDrawSpacesFlags.TAB | SourceDrawSpacesFlags.SPACE;
		this.smart_home_end = SourceSmartHomeEndType.BEFORE;
		this.show_line_numbers = Settings.get_show_line_numbers ();
		this.show_right_margin = Settings.get_show_right_margin ();
		
		this.cut_clipboard += on_cut_clipboard;
		this.copy_clipboard += on_copy_clipboard;
		this.paste_clipboard += on_paste_clipboard;
		this.focus_out_event += on_focus_out;
	}
	
	private void save_state () {
		TextIter start_iter, end_iter;
		
		if ( document.get_selection_bounds (out start_iter, out end_iter) ) {
			int tmp = end_iter.get_offset ();
			document.metadata.@set ("sel_offset", tmp.to_string ());
		} else {
			document.metadata.@set ("sel_offset", "-1");
		}
		int tmp = start_iter.get_offset ();
		document.metadata.@set ("cur_offset", tmp.to_string ());
	}
	
	private void restore_state () {
		TextIter start_iter, end_iter;
		
		int cur = document.metadata.get_int ("cur_offset");
		int sel = document.metadata.get_int ("sel_offset");
		
		document.get_iter_at_offset (out start_iter, cur);
		if ( sel > -1 ) {
			document.get_iter_at_offset (out end_iter, sel);
		} else {
			end_iter = start_iter;
		}
		document.select_range (start_iter, end_iter);
	}
	
	public bool find (string text, TextIter start, bool match_case = false, bool backwards = false, bool wrap = true) {
		return_if_fail (document != null);
	
		TextIter match;
		TextIter match_end;
		SourceSearchFlags search_flags;
		bool wrapped = false;
		bool found = false;
		
		if ( !match_case ) {
			search_flags |= SourceSearchFlags.CASE_INSENSITIVE;
		}
		
		TextIter limit;	
		if ( backwards )
			document.get_start_iter (out limit);
		else
			document.get_end_iter (out limit);
		
		while (true) {
			if ( backwards ) {
				found = Gtk.source_iter_backward_search (start, text, search_flags, out match, out match_end, limit);
			} else {
				found = Gtk.source_iter_forward_search (start, text, search_flags, out match, out match_end, limit);
			}
		
			if ( found ) {
				document.select_range (match, match_end);
				scroll_to_iter (match_end, 0, false, 0, 0);
				return true;
			} else {
				if ( !wrap || wrapped ) {
					return false;
				}
			}
			
			if ( wrap && !wrapped) {
				limit = start;
				if ( backwards )
					document.get_end_iter (out start);
				else
					document.get_start_iter (out start);
				wrapped = true;
			}
		};

		return false;
	}
	
	public void select_all () {
		return_if_fail (document != null);
		TextIter s, e;
		document.get_bounds (out s, out e);
		document.select_range (s, e);
	}
	
	private string normalize_block_clip (string clipstring) {
		string ret = clipstring;
		if ( !clipstring.has_prefix ("\n") )
				ret = "\n" + ret;
		return ret + "\n";	
	}
	
	private void save_selection () {
		TextIter start, end;
		document.get_selection_bounds (out start, out end);
		document.create_mark ("saved_insert", start, true);
		document.create_mark ("saved_selection_bound", end, true);
	}
	
	private void restore_selection () {
		TextIter start, end;
		document.get_iter_at_mark (out start, document.get_mark ("saved_insert"));
		document.get_iter_at_mark (out end, document.get_mark ("saved_selection_bound"));
		document.select_range (start, end);
	}
	
	private void select_block (TextIter start, TextIter end) {
		start.set_line_index (0);
		if ( !end.ends_line () )
			end.forward_to_line_end ();
		
		if ( start.get_line () > 0 )
			start.backward_char ();
		else
			end.forward_char ();
		
		document.select_range (start, end);
	}
	
	public void block_cut_clipboard (bool block = false) {
		return_if_fail (document != null);
	
		TextIter start, end;	
		bool has_selection = document.get_selection_bounds (out start, out end);
		
		if ( has_selection && !block ) {
			document.cut_clipboard (BaseWindow.get_clipboard (), true);
		} else {
			int line = start.get_line ();
			int offset = start.get_line_offset ();
			select_block (start, end);
			string clipstring = normalize_block_clip (document.get_text (start, end, false));
			document.delete_interactive (start, end, true);
			
			document.get_iter_at_line (out start, line);
			if ( offset > start.get_chars_in_line ()-1 )
				start.set_line_offset (start.get_chars_in_line ()-1);
			else
				start.set_line_offset (offset);
			document.select_range (start, start);
			
			BaseWindow.get_clipboard ().set_text (clipstring, -1);
		}
	}
	
	public void block_copy_clipboard (bool block = false) {
		return_if_fail (document != null);
	
		TextIter start, end;
		bool has_selection = document.get_selection_bounds (out start, out end);
		
		if ( has_selection && !block ) {
			buffer.copy_clipboard (BaseWindow.get_clipboard ());
		} else {
			save_selection ();
			select_block (start, end);
			restore_selection ();
			string clipstring = normalize_block_clip (document.get_text (start, end, false));
			BaseWindow.get_clipboard ().set_text (clipstring, -1);
		}
	}
	
	public void block_paste_clipboard () {
		return_if_fail (document != null);
	
		TextIter iter, end;
		string clipstring = BaseWindow.get_clipboard ().wait_for_text ();
		
		document.get_selection_bounds (out iter, out end);
		
		if ( clipstring.has_prefix ("\n") && clipstring.has_suffix ("\n") ) {
			StringBuilder str = new StringBuilder (BaseWindow.get_clipboard ().wait_for_text ());
			str.erase (str.len -1, 1);
			if ( !iter.ends_line () )
				iter.forward_to_line_end ();
			document.insert_interactive (iter, str.str, -1, true);
		} else {
			document.paste_clipboard (BaseWindow.get_clipboard (), iter, true);
		}
	}
	
	public int get_current_line () {
		return_if_fail (document != null);
	
		TextIter iter;
		document.get_iter_at_mark (out iter, document.get_insert ());
		return iter.get_line ();
	}
	
	public void go_to_line (int linenum) {
		TextIter iter;
		
		if ( linenum <= 0 ) return;
		
		document.get_iter_at_mark (out iter, document.get_insert ());
		iter.set_line (linenum - 1);
		document.place_cursor (iter);
		scroll_to_iter (iter, 0, false, 0, 0);
	}
	
	public void insert_new_line () {
		TextIter iter;
		unichar ichar;
		StringBuilder indent = new StringBuilder ("");
		string tmp;
		// save the indentation of the current line
		document.get_iter_at_mark (out iter, document.get_insert ());
		iter.set_line_offset (0);
		while ( iter.get_char () == '\t' || iter.get_char () == ' ' ) {
			ichar = iter.get_char ();
			indent.append_unichar (ichar);
			iter.forward_char ();
		}
		
		// move the cursor to the end of the line and insert a newline character with indentation
		if ( !iter.ends_line () ) iter.forward_to_line_end ();
		document.insert_interactive (iter, "\n" + indent.str, -1, true);
		document.get_iter_at_mark (out iter, document.get_insert ());
		if ( !iter.ends_line () ) {
			iter.forward_to_line_end ();
			iter.forward_to_line_end ();
		}
		document.place_cursor (iter);
	}
	
	public void tag_selection () {
		TextIter s, e;
		
		if (document.get_selection_bounds (out s, out e)) {
			add_tag (s, e);
		}
	}
	
	
	public void tag_and_find_next () {
		TextIter s, e;
		
		if (document.get_selection_bounds (out s, out e)) {
			add_tag (s, e);
			find (document.get_text (s, e, false), e, true);
		}
	}
	
	public void tag_search_results (string str) {
		TextIter s, e;
		TextIter match, match_end;
		
		document.get_bounds (out s, out e);
			
		while ( source_iter_forward_search (s, str, (SourceSearchFlags)0, out match, out match_end, e) ) {
			add_tag (match, match_end);
			s = match_end;
		}
	}
	
	public void remove_tags () {
		TextIter s, e;
		int os, oe, ts, te;
		
		if (!document.get_selection_bounds (out s, out e))
			document.get_bounds (out s, out e);
			
		os = s.get_offset ();
		oe = e.get_offset ();
		
		SList<Tag> tmp_list;
		foreach (Tag tag in tag_list) {
			document.get_iter_at_mark (out s, tag.begin);
			document.get_iter_at_mark (out e, tag.end);
			ts = s.get_offset ();
			te = e.get_offset ();
			
			if ( ts < os || te > oe ) {
				tmp_list.append (tag);
			}
		}
		tag_list = #tmp_list;
		
		update_tags ();
	}
	
	public void replace (string str) {
		TextIter s, e;
		
		document.begin_user_action ();
		foreach (Tag tag in tag_list) {
			document.get_iter_at_mark (out s, tag.begin);
			document.get_iter_at_mark (out e, tag.end);
			document.delete (s, e);
			document.insert_interactive (s, str, -1, true);
			document.get_iter_at_mark (out s, tag.begin);
			document.get_iter_at_mark (out e, tag.end);
			document.apply_tag_by_name ("tag", s, e);
		}
		document.end_user_action ();
	}
	
	private void add_tag (TextIter s, TextIter e) {
		document.apply_tag_by_name ("tag", s, e);
		var tag = new Tag ();
		tag.begin = document.create_mark (null, s, true);
		tag.end = document.create_mark (null, e, false);
		tag_list.append (tag);	
	}
	
	private void update_tags () {
		TextIter s, e;
		
		document.get_bounds (out s, out e);
		document.remove_tag_by_name ("tag", s, e);
		foreach (Tag tag in tag_list) {
			document.get_iter_at_mark (out s, tag.begin);
			document.get_iter_at_mark (out e, tag.end);
			document.apply_tag_by_name ("tag", s, e);
		}
	}
	
	private void on_font_changed (uint cnxn_id, GConf.Entry entry) {
		stdout.printf ("font changed\n");
	}
	
	private bool on_focus_out (GLib.Object o, Gdk.EventFocus e) {
		if ( document == null ) {
			return false;
		}
		
		// save document state
		save_state ();
		if ( wrap_mode == Gtk.WrapMode.NONE ) {
			document.metadata.@set ("word-wrap", "false");
		} else {
			document.metadata.@set ("word-wrap", "true");
		}
		document.metadata.write ();
		
		document.save ();
		return false;
	}
	
	/* these need to be default handler overrides instead, otherwise things are getting pasted twice... */
	private void on_cut_clipboard (GLib.Object ow) {
		stdout.printf("cut\n");
		//block_cut_clipboard ();
	}
	
	private void on_copy_clipboard (GLib.Object o) {
		stdout.printf("copy\n");
		//block_copy_clipboard ();
	}
	
	private void on_paste_clipboard (GLib.Object o) {
		stdout.printf("past\n");
		//block_paste_clipboard ();
	}
}
