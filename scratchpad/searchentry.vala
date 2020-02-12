using GLib;
using Gtk;
using Gdk;

public enum SearchMode {
	SILENT,
	TEXT,
	GO_TO_LINE,
	REPLACE,
}

public class SearchEventArgs : GLib.Object
{
	public bool backwards = false;
	public bool next_result = false;
	public string search_text;
	public SearchMode mode {
		get { return _mode; }
		set { _mode = value; }
	}
	private SearchMode _mode; // this can't be public yet due to a vala ordering bug
}

public class SearchEntry : Entry
{
	public signal void search_activated (SearchEventArgs args);
	public signal void search_closed ();
	
	private SearchMode mode = SearchMode.TEXT;
	public Label desc_label {get; set construct;}
	private int maxlines = 9999; // used in go to mode
	
	private string last_search = "";
	private string last_replace = "";
	
	public SearchEntry (Label desc_label) {
		this.desc_label = desc_label;
	}
	
	construct {
		key_press_event += on_key_press_event;
		changed += on_changed;
	}
	
	public string get_last_search () {
		return last_search;
	}
	
	public string get_last_replace () {
		return last_replace;
	}
	
	public void set_maxlines (int lines) {
		maxlines = lines;
	}
	
	public void set_mode (SearchMode new_mode) {
		mode = SearchMode.SILENT;
		if ( new_mode == SearchMode.TEXT ) {
			desc_label.set_text ("Find:");
			set_text (last_search);
		} else if ( new_mode == SearchMode.GO_TO_LINE ) {
			desc_label.set_text ("Go to:");
		} else if ( new_mode == SearchMode.REPLACE ) {
			desc_label.set_text ("Replace:");
			set_text (last_replace);
		}
		mode = new_mode;
	}
	
	private void on_changed (SearchEntry sender) {
		if ( mode == SearchMode.SILENT )
			return;
			
		// we don't replace-as-you-type as it would fill up the undo stack
		if ( mode == SearchMode.REPLACE )
			return;
	
		var args = new SearchEventArgs ();
		args.mode = mode;
		args.search_text = get_text ();
		
		if ( mode == SearchMode.TEXT ) {
			last_search = args.search_text;
		} else if ( mode == SearchMode.REPLACE ) {
			last_replace = args.search_text;
		}
		
		search_activated (args);
	}
	
	private int clamp (int val) {
		if ( val < 1 )
			return 1;
		if ( val > maxlines )
			return maxlines;
		return val;
	}
	
	private bool on_key_press_event (SearchEntry sender, EventKey key) {	
		if ( mode == SearchMode.SILENT )
			return false;
	
		var args = new SearchEventArgs ();
		args.mode = mode;
		args.next_result = true;
		args.search_text = get_text ();
		
		if ( mode == SearchMode.TEXT ) {
			last_search = args.search_text;
		} else if ( mode == SearchMode.REPLACE ) {
			last_replace = args.search_text;
		}
	
		if ( keyval_name(key.keyval) == "Return" || keyval_name(key.keyval) == "Down" || keyval_name(key.keyval) == "Up" ) {
			// we don't replace-as-you-type as it would fill up the undo stack
			if ( mode == SearchMode.REPLACE ) {
				if ( keyval_name(key.keyval) == "Return" ) {
					search_activated (args);
					search_closed ();
					return true;
				}
				return false;
			}
			
			if ( mode == SearchMode.GO_TO_LINE ) {
				if ( keyval_name(key.keyval) == "Return" ) {
					search_closed ();
					return true;
				}
			
				int adjust = 0;
				if ( keyval_name(key.keyval) == "Up" )
					adjust = 10;
				if ( keyval_name(key.keyval) == "Down" )
					adjust = -10;
				if ( key.state == 1 /*shift pressed*/ )
					adjust *= 10;
				
				// great fun!
				set_text (clamp ((get_text ().to_int () + adjust)).to_string ());
				grab_focus ();
				return true;
			}
		
			if ( keyval_name(key.keyval) == "Up" || key.state == 1 /*shift pressed*/ )
				args.backwards = true;
			search_activated (args);
			return true;
		}
		
		return false;
	}
}
