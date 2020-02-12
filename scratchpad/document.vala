using GLib;
using Gtk;

public class Document : SourceBuffer
{
	public string uri { get; set construct; }
	
	private string file_name;
	private string mime_type;
	private string modified;
	
	public MetadataManager metadata { get; set; }


	public Document ( string uri ) {
		this.uri = uri;
		this.highlight_syntax = true;
	}
	
	construct {
		SourceLanguage lang;
		bool result_uncertain = false;
	
		this.file_name = uri_to_filename (uri);
		this.metadata = new MetadataManager (Uri.escape_string (this.file_name, null, false));
		this.mime_type = g_content_type_guess (this.file_name, null, out result_uncertain);
		
		if (result_uncertain)
			this.mime_type = null;
		
		var slm = new SourceLanguageManager ();
		
		if (result_uncertain)
			lang = slm.guess_language (this.file_name, null);
		else
			lang = slm.guess_language (null, this.mime_type);

		if ( lang != null ) {
			set_language (lang);
		}
	}
	
	public bool open () {
		// TODO: error dialogs, use gio
		string contents;
		
		if ( ! FileUtils.get_contents (file_name, out contents, null) )
			return false;
		
		if ( ! contents.validate () ) {
			/* this is lame .. */
			contents = convert (contents, -1, "UTF-8", "ISO-8859-2", null, null);
			if ( ! contents.validate () ) {
				return false;
			}
		}
		
		begin_not_undoable_action ();
		set_text (contents, -1);
		end_not_undoable_action ();
		set_modified (false);
		return true;
	}
	
	public void save () {
		return_if_fail (this != null);
	
		if ( !get_modified () ) return;
		
		string buffer;
		buffer = Application.get_config_dir () + "/save_buffer";
		
		TextIter start, end;
		get_bounds (out start, out end);
		string contents = get_text (start, end, false);

		try { 
			FileUtils.set_contents (buffer, contents, -1);
		} catch (FileError err) {
			stdout.printf ("Error: Could not save document");
			return;
		}
		
		if ( FileUtils.rename (buffer, file_name) == -1 ) {
			stdout.printf ("Error: Could not save document");
			return;
		}
		
		set_modified (false);
	}
	
	public string get_file_name () {
		return file_name;
	}
	
	public string get_mimetype () {
		return mime_type;
	}
	
	public string get_folder () {
		var parts = file_name.reverse ().split ("/", 2);
		return parts[1].reverse ();
	}
	
	public string get_basename () {
		return Filename.display_basename (file_name);
	}
	
	public string get_title_string () {
		string folder = get_folder ();
		string home = Environment.get_home_dir ();
		if ( folder == home ) {
			folder = "";
		} else if ( folder.has_prefix (home) ) {
			folder = " (" + folder.offset (home.len ()+1) + ")";
		} else {
			folder = " (" + folder + ")";
		}
	
		return get_basename () + folder;
	}
	
	public static string uri_to_filename (string uri) {
		string f = uri;
	
		// only local URIs are supported for now
		if ( uri.has_prefix ("file:") )
			f = uri.split ("://")[1];
		else if ( ! Path.is_absolute (uri) )
			f = Path.build_filename (Environment.get_current_dir (), uri);
		
		return f;
	}
}
