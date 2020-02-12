using GConf;

namespace Settings {
	public static bool get_show_line_numbers () {
		bool res;
		try {
			res = GConf.Client.get_default ().get_bool ("/apps/scratchpad/show_line_numbers");
		} catch (GLib.Error error) {
			stdout.printf ("Error: %s\n", error.message);
			return false;
		}
		return res;
	}
	public static void set_show_line_numbers (bool val) {
		try {
			GConf.Client.get_default ().set_bool ("/apps/scratchpad/show_line_numbers", val);
		} catch (GLib.Error error) {
			stdout.printf ("Error: %s\n", error.message);
		}
	}
	
	public static bool get_show_right_margin () {
		bool res;
		try {
			res =  GConf.Client.get_default ().get_bool ("/apps/scratchpad/show_margin");
		} catch (GLib.Error error) {
			stdout.printf ("Error: %s\n", error.message);
			return false;
		}
		return res;
	}
	public static void set_show_right_margin (bool val) {
		try {
			GConf.Client.get_default ().set_bool ("/apps/scratchpad/show_margin", val);
		} catch (GLib.Error error) {
			stdout.printf ("Error: %s\n", error.message);
		}
	}
}

namespace SystemSettings {
	public static string get_font () {
		string res;
		try {
			res = GConf.Client.get_default ().get_string ("/desktop/gnome/interface/monospace_font_name");
		} catch (GLib.Error error) {
			stdout.printf ("Error: %s\n", error.message);
		}
		return res;
	}
}
