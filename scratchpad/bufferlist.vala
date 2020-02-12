using GLib;
using Gtk;

public class BufferList : TreeView {

	ProjectWindow window;
	//Project project;
	ListStore store;
	TreeViewColumn main_column;
	
	public BufferList (ProjectWindow win) {
	}
	
	construct {
		/*
		0 - Icon
		1 - Label
		2 - Uri
		*/
		//store = new ListStore (typeof (Gdk.Pixbuf), typeof (string), typeof (string));
		//model = store;
	
		main_column = new TreeViewColumn ();
		main_column.title = "Buffer List";
		
		append_column (main_column);
	}
	
}