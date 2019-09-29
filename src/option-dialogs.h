#ifndef OPTION_DIALOGS_H
#define OPTION_DIALOGS_H

#include <unordered_map>
#include <string>

#include "config.h"
#include "utils.h"
#include "widgets.h"

#pragma warning(push, 0)
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Native_File_Chooser.H>
#pragma warning(pop)

#define NEW_TILEMAP_NAME "New Tilemap"

class Option_Dialog {
protected:
	int _width;
	const char *_title;
	bool _canceled;
	Fl_Double_Window *_dialog;
	Fl_Group *_content;
	Default_Button *_ok_button;
	OS_Button *_cancel_button;
public:
	Option_Dialog(int w, const char *t = NULL);
	virtual ~Option_Dialog();
	inline bool canceled(void) const { return _canceled; }
	inline void canceled(bool c) { _canceled = c; }
protected:
	void initialize(void);
	void refresh(void);
	virtual void initialize_content(void) = 0;
	virtual int refresh_content(int ww, int dy) = 0;
	virtual void finish(void) {}
public:
	inline bool initialized(void) const { return !!_dialog; }
	void show(const Fl_Widget *p);
private:
	static void close_cb(Fl_Widget *, Option_Dialog *od);
	static void cancel_cb(Fl_Widget *, Option_Dialog *od);
};

class Tilemap_Options_Dialog : public Option_Dialog {
private:
	Label *_tilemap_header;
	Dropdown *_format;
public:
	Tilemap_Options_Dialog(const char *t);
	~Tilemap_Options_Dialog();
	inline Tilemap_Format format(void) const { return (Tilemap_Format)_format->value(); }
	inline void format(Tilemap_Format fmt) { initialize(); _format->value((int)fmt); }
	void use_tilemap(const char *filename);
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
};

class New_Tilemap_Dialog : public Option_Dialog {
private:
	OS_Spinner *_tilemap_width, *_tilemap_height;
	Dropdown *_format;
public:
	New_Tilemap_Dialog(const char *t);
	~New_Tilemap_Dialog();
	inline size_t tilemap_width(void) const { return (size_t)_tilemap_width->value(); }
	inline void tilemap_width(size_t n) { initialize(); _tilemap_width->value((double)n); }
	inline size_t tilemap_height(void) const { return (size_t)_tilemap_height->value(); }
	inline void tilemap_height(size_t n) { initialize(); _tilemap_height->value((double)n); }
	inline Tilemap_Format format(void) const { return (Tilemap_Format)_format->value(); }
	inline void format(Tilemap_Format fmt) { initialize(); _format->value((int)fmt); }
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
};

class Anchor_Button;

class Resize_Dialog : public Option_Dialog {
public:
	enum Hor_Align { LEFT, CENTER, RIGHT };
	enum Vert_Align { TOP, MIDDLE, BOTTOM };
private:
	Default_Spinner *_tilemap_width, *_tilemap_height;
	Anchor_Button *_anchor_buttons[9];
public:
	Resize_Dialog(const char *t);
	~Resize_Dialog();
	inline size_t tilemap_width(void) const { return (size_t)_tilemap_width->value(); }
	inline size_t tilemap_height(void) const { return (size_t)_tilemap_height->value(); }
	inline void tilemap_size(size_t w, size_t h) {
		initialize();
		_tilemap_width->default_value(w);
		_tilemap_height->default_value(h);
	}
	Hor_Align horizontal_anchor(void) const;
	Vert_Align vertical_anchor(void) const;
	int anchor(void) const;
	void anchor(int a);
	void anchor_label(int x, int y, const char *l);
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
private:
	static void anchor_button_cb(Anchor_Button *ab, Resize_Dialog *rd);
};

class Tilemap_Width_Dialog : public Option_Dialog {
private:
	OS_Spinner *_tilemap_width;
public:
	Tilemap_Width_Dialog(const char *t);
	~Tilemap_Width_Dialog();
	inline size_t tilemap_width(void) const { return (size_t)_tilemap_width->value(); }
	inline void tilemap_width(size_t n) { initialize(); _tilemap_width->value((double)n); }
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
};

class Add_Tileset_Dialog : public Option_Dialog {
private:
	Label *_tileset_header;
	Default_Hex_Spinner *_start_id;
	Default_Spinner *_offset, *_length;
public:
	Add_Tileset_Dialog(const char *t);
	~Add_Tileset_Dialog();
	inline int start_id(void) const { return _start_id->value(); }
	inline void start_id(int n) { initialize(); _start_id->value(n); }
	inline int offset(void) const { return (int)_offset->value(); }
	inline void offset(int n) { initialize(); _offset->value((double)n); }
	inline int length(void) const { return (int)_length->value(); }
	inline void length(int n) { initialize(); _length->value((double)n); }
	void limit_tileset_options(const char *filename);
protected:
	void initialize_content(void);
	int refresh_content(int ww, int dy);
};

class Image_To_Tiles_Dialog : public Option_Dialog {
private:
	Label *_image_heading, *_tilemap_heading, *_tileset_heading;
	Toolbar_Button *_image, *_tilemap, *_tileset;
	Label_Button *_image_name, *_tilemap_name, *_tileset_name;
	Dropdown *_format;
	Default_Hex_Spinner *_start_id;
	OS_Check_Button *_use_7f;
	Fl_Native_File_Chooser *_image_chooser, *_tilemap_chooser, *_tileset_chooser;
	std::string _image_filename, _tilemap_filename, _tileset_filename;
public:
	Image_To_Tiles_Dialog(const char *t);
	~Image_To_Tiles_Dialog();
	inline const char *image_filename(void) const { return _image_filename.c_str(); }
	inline const char *tilemap_filename(void) const { return _tilemap_filename.c_str(); }
	inline const char *tileset_filename(void) const { return _tileset_filename.c_str(); }
	inline Tilemap_Format format(void) const { return (Tilemap_Format)_format->value(); }
	inline void format(Tilemap_Format fmt) { initialize(); _format->value((int)fmt); }
	inline uint16_t start_id(void) const { return (uint16_t)_start_id->value(); }
	inline void start_id(uint16_t n) { initialize(); _start_id->value(n); }
	inline bool use_7f(void) const { return !!_use_7f->value(); }
private:
	void update_ok_button(void);
protected:
	void initialize_content(void);
	void refresh(void);
	int refresh_content(int ww, int dy);
private:
	static void image_cb(Fl_Widget *w, Image_To_Tiles_Dialog *itd);
	static void tilemap_cb(Fl_Widget *w, Image_To_Tiles_Dialog *itd);
	static void tileset_cb(Fl_Widget *w, Image_To_Tiles_Dialog *itd);
};

#endif
