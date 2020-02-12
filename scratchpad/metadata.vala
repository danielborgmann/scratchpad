using GLib;

public class MetadataManager : Object
{
	// temporary hack until we can iterate through hashtables
	private const string[] keynames = { "x", "y", "width", "height",
	                                    "hscroll", "vscroll",
	                                    "cur_offset", "sel_offset",
	                                    "word-wrap" };

	private string file_name;
	private HashTable<string,string> keys = new HashTable<string,string> (str_hash, str_equal);
	private bool modified = false;
	public string hash {get; set construct;}
	
	public MetadataManager (string hash) {
		this.hash = hash;
	}
	
	construct {
		string contents;
		
		file_name = Application.get_config_dir() + "/metadata/" + hash;
		
		if ( FileUtils.test (file_name, FileTest.EXISTS) ) {
			FileUtils.get_contents (file_name, out contents, null); // FIXME: use async gio?
			var lines = contents.split ("\n");
			
			foreach ( string line in lines ) {
				var keyval = line.split (":");
				if ( keyval[0] == null || keyval[1] == null )
					continue;
				keys.insert (keyval[0], keyval[1]);
			}
		}
	}
	
	public string @get (string key) {
		string val = keys.lookup (key);
		if (val != null) {
			return val;
		} else {
			return "";
		}
	}
	
	public int get_int (string key) {
		string r = keys.lookup (key);
		if ( keys.lookup (key) != null )
			return r.to_int ();
		else
			return -1;
	}
	
	public long get_long (string key) {
		string r = keys.lookup (key);
		if ( keys.lookup (key) != null )
			return (long)r.to_int64 ();
		else
			return -1;
	}
	
	public void @set (string key, string value) {
		keys.replace (key, value);
		modified = true;
	}
	
	public void write () {
		if ( !modified ) return;
	
		string output = "";
		Error err;
		
		foreach (string key in keynames) {
			string val = keys.lookup (key);
			if ( val != null ) {
				output = output + key + ":" + val + "\n";
			}
		}
		
		try { 
			FileUtils.set_contents (file_name, output, -1);
		}
		catch (FileError e) {
			stdout.printf ("Warning: Could not save metadata\n");
		}
		
		modified = false;
	}
}
