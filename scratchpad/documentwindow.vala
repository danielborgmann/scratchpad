using GLib;
using Gtk;

public class DocumentWindow : BaseWindow
{
	public string filename {get; set construct;}
	
	public DocumentWindow (string filename) {
		this.filename = filename;
	}

	construct {
		populate_window ();
		
		var doc = new Document (filename);
		doc.open ();
		document_view.document = doc;
		
		connect_events ();
		restore_spatial_state ();
		restore_scroll_position ();
		update_menu_state ();
		set_title (doc.get_title_string ());
		
		// FIXME: should use document mimetype icon (wait for Gtk 2.14 and nicer API)
		var icon = IconTheme.get_default ().load_icon ("scratchpad", 16, (IconLookupFlags)0);
		set_icon (icon);
	}
		
	public override bool is_document (string uri) {
		return ( document_view.document.get_file_name () == Document.uri_to_filename (uri) );
	}
	
	private void restore_spatial_state () {
		var m = document_view.document.metadata;
		
		int x = m.get_int ("x");
		int y = m.get_int ("y");
		int width = m.get_int ("width");
		int height = m.get_int ("height");
		
		if ( x != -1 && y != -1 ) {
			move (x, y);
		}
		
		if ( width != -1 && height != -1 ) {
			resize (width, height);
		} else {
			resize (780, 600);
		}
	}
	
	private void save_spatial_state () {
		var m = document_view.document.metadata;
		
		int width, height, x, y;
		get_size (out width, out height);
		get_position (out x, out y);
		
		m.@set ("x", x.to_string ());
		m.@set ("y", y.to_string ());
		m.@set ("width", width.to_string ());
		m.@set ("height", height.to_string ());
		
		m.write ();
	}
	
	private void populate_window () {
		base.populate_window ();
		
		document_view = new DocumentView ();
		
		scrolled_window.add (document_view);
		document_box.add (scrolled_window);
	}
	
	private void connect_events () {
		delete_event += () => { save_spatial_state (); return false; }; 
		base.connect_events ();
	}
}
