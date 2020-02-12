using GLib;

/*
 * Project is the representation of a project document on the users filesystem.
 * Project settings, folders, etc should be stored here.
 */
public class Project : GLib.Object
{
	
	public string uri {get; set construct;}
	
	public Project (string uri) {
		this.uri = uri;
	} 
	
	construct {
		/* Create the project document if it doesn't exist. */
		var file = File.new_for_commandline_arg (this.uri);
		
		try {
			file.create (FileCreateFlags.NONE, null);
		} catch (IOError e) {
			if (e is IOError.EXISTS)
				stdout.printf ("File Exists\n");
			else
				stdout.printf ("Error: %s\n", e.message);
		}
	}
	
}