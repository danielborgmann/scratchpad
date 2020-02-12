using GLib;
using Gtk;

public class ProjectTree : TreeView {
	construct {
		//this.drag_dest_set (DestDefaults.ALL, null, 
		this.drag_data_received += on_drag_data_received;
	}
	
	void on_drag_data_received () {
		stdout.printf ("drag data received\n");
	}
}
