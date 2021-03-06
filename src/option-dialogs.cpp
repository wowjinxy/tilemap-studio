#include <cctype>
#include <cstdlib>

#pragma warning(push, 0)
#include <FL/Enumerations.H>
#include <FL/Fl_Double_Window.H>
#pragma warning(pop)

#include "preferences.h"
#include "themes.h"
#include "widgets.h"
#include "utils.h"
#include "option-dialogs.h"
#include "tileset.h"
#include "config.h"
#include "icons.h"
#include "image.h"

Option_Dialog::Option_Dialog(int w, const char *t) : _width(w), _title(t), _canceled(false),
	_dialog(NULL), _content(NULL), _ok_button(NULL), _cancel_button(NULL) {}

Option_Dialog::~Option_Dialog() {
	delete _dialog;
	delete _content;
	delete _ok_button;
	delete _cancel_button;
}

void Option_Dialog::initialize() {
	if (_dialog) { return; }
	Fl_Group *prev_current = Fl_Group::current();
	Fl_Group::current(NULL);
	// Populate dialog
	_dialog = new Fl_Double_Window(0, 0, 0, 0, _title);
	_content = new Fl_Group(0, 0, 0, 0);
	_content->begin();
	initialize_content();
	_content->end();
	_dialog->begin();
	_ok_button = new Default_Button(0, 0, 0, 0, "OK");
	_cancel_button = new OS_Button(0, 0, 0, 0, "Cancel");
	_dialog->end();
	// Initialize dialog
	_dialog->resizable(NULL);
	_dialog->callback((Fl_Callback *)cancel_cb, this);
	_dialog->set_modal();
	// Initialize dialog's children
	_ok_button->tooltip("OK (Enter)");
	_ok_button->callback((Fl_Callback *)close_cb, this);
	_cancel_button->shortcut(FL_Escape);
	_cancel_button->tooltip("Cancel (Esc)");
	_cancel_button->callback((Fl_Callback *)cancel_cb, this);
	Fl_Group::current(prev_current);
}

void Option_Dialog::refresh() {
	_canceled = false;
	_dialog->copy_label(_title);
	// Refresh widget positions and sizes
	fl_font(OS_FONT, OS_FONT_SIZE);
	int dy = 10;
	dy += refresh_content(_width - 20, dy) + 16;
#ifdef _WIN32
	_ok_button->resize(_width - 184, dy, 80, 22);
	_cancel_button->resize(_width - 90, dy, 80, 22);
#else
	_cancel_button->resize(_width - 184, dy, 80, 22);
	_ok_button->resize(_width - 90, dy, 80, 22);
#endif
	dy += _cancel_button->h() + 10;
	_dialog->size_range(_width, dy, _width, dy);
	_dialog->size(_width, dy);
	_dialog->redraw();
}

void Option_Dialog::show(const Fl_Widget *p) {
	initialize();
	refresh();
	int x = p->x() + (p->w() - _dialog->w()) / 2;
	int y = p->y() + (p->h() - _dialog->h()) / 2;
	_dialog->position(x, y);
	_ok_button->take_focus();
	_dialog->show();
	while (_dialog->shown()) { Fl::wait(); }
}

void Option_Dialog::close_cb(Fl_Widget *, Option_Dialog *od) {
	od->finish();
	od->_dialog->hide();
}

void Option_Dialog::cancel_cb(Fl_Widget *, Option_Dialog *od) {
	od->_canceled = true;
	od->_dialog->hide();
}

Tilemap_Options_Dialog::Tilemap_Options_Dialog(const char *t) : Option_Dialog(280, t), _tilemap_header(NULL),
	_format(NULL), _attrmap_heading(NULL), _attrmap(NULL), _attrmap_name(NULL), _attrmap_chooser(NULL),
	_attrmap_filename() {}

Tilemap_Options_Dialog::~Tilemap_Options_Dialog() {
	delete _tilemap_header;
	delete _format;
	delete _attrmap_heading;
	delete _attrmap;
	delete _attrmap_name;
	delete _attrmap_chooser;
}

void Tilemap_Options_Dialog::update_icons() {
	initialize();
	Image::make_deimage(_attrmap);
}

void Tilemap_Options_Dialog::use_tilemap(const char *filename) {
	initialize();
	const char *name = fl_filename_name(filename);
	char buffer[FL_PATH_MAX] = {};
	strcpy(buffer, name);
	strcat(buffer, ":");
	_tilemap_header->copy_label(buffer);

	format(guess_format(filename));
	if (format_has_attrmap(format())) {
		strcpy(buffer, filename);
		fl_filename_setext(buffer, sizeof(buffer), ATTRMAP_EXT);
		_attrmap_filename = buffer;
		_attrmap_heading->activate();
		_attrmap->activate();
		_attrmap_name->activate();
		const char *basename = fl_filename_name(buffer);
		_attrmap_name->copy_label(basename);
	}
	else {
		_attrmap_filename.clear();
		_attrmap_heading->deactivate();
		_attrmap->deactivate();
		_attrmap_name->deactivate();
		_attrmap_name->copy_label(NO_FILE_SELECTED_LABEL);
	}
}

void Tilemap_Options_Dialog::initialize_content() {
	// Populate content group
	_tilemap_header = new Label(0, 0, 0, 0);
	_format = new Dropdown(0, 0, 0, 0, "Format:");
	_attrmap_heading = new Label(0, 0, 0, 0, "Attrmap:");
	_attrmap = new Toolbar_Button(0, 0, 0, 0);
	_attrmap_name = new Label_Button(0, 0, 0, 0, NO_FILE_SELECTED_LABEL);
	_attrmap_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	// Initialize content group's children
	for (int i = 0; i < NUM_FORMATS; i++) {
		_format->add(format_name((Tilemap_Format)i));
	}
	_format->callback((Fl_Callback *)format_cb, this);
	_attrmap->callback((Fl_Callback *)attrmap_cb, this);
	_attrmap->image(OPEN_ICON);
	_attrmap_name->callback((Fl_Callback *)attrmap_cb, this);
	_attrmap_chooser->title("Open Attrmap");
	_attrmap_chooser->filter("Attrmap Files\t*.attrmap\n");
}

int Tilemap_Options_Dialog::refresh_content(int ww, int dy) {
	int wgt_h = 22, win_m = 10, wgt_m = 4;
	int ch = (wgt_h + wgt_m) * 2 + wgt_h;
	_content->resize(win_m, dy, ww, ch);

	_tilemap_header->resize(win_m, dy, ww, wgt_h);
	dy += wgt_h + wgt_m;
	int wgt_off = win_m + text_width(_format->label(), 2);
	int wgt_w = format_max_name_width() + wgt_h;
	_format->resize(wgt_off, dy, wgt_w, wgt_h);
	dy += wgt_h + wgt_m;
	wgt_w = text_width(_attrmap_heading->label(), 4);
	wgt_off = win_m;
	_attrmap_heading->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off += _attrmap_heading->w();
	_attrmap->resize(wgt_off, dy, wgt_h, wgt_h);
	wgt_off += _attrmap->w();
	_attrmap_name->resize(wgt_off, dy, ww-wgt_w-wgt_h, wgt_h);

	return ch;
}

void Tilemap_Options_Dialog::format_cb(Dropdown *, Tilemap_Options_Dialog *tod) {
	if (format_has_attrmap(tod->format())) {
		tod->_attrmap_heading->activate();
		tod->_attrmap->activate();
		tod->_attrmap_name->activate();
		const char *filename = tod->_attrmap_chooser->filename();
		if (filename && strlen(filename)) {
			const char *basename = fl_filename_name(filename);
			tod->_attrmap_filename = filename;
			tod->_attrmap_name->copy_label(basename);
		}
		else {
			tod->_attrmap_filename.clear();
			tod->_attrmap_name->copy_label(NO_FILE_SELECTED_LABEL);
		}
	}
	else {
		tod->_attrmap_filename.clear();
		tod->_attrmap_heading->deactivate();
		tod->_attrmap->deactivate();
		tod->_attrmap_name->deactivate();
		tod->_attrmap_name->copy_label(NO_FILE_SELECTED_LABEL);
	}
	tod->_dialog->redraw();
}

void Tilemap_Options_Dialog::attrmap_cb(Fl_Widget *, Tilemap_Options_Dialog *tod) {
	int status = tod->_attrmap_chooser->show();
	if (status == 1) {
		tod->_attrmap_filename.clear();
		tod->_attrmap_name->label(NO_FILE_SELECTED_LABEL);
	}
	else {
		const char *filename = tod->_attrmap_chooser->filename();
		const char *basename = fl_filename_name(filename);
		tod->_attrmap_filename = filename;
		tod->_attrmap_name->copy_label(basename);
	}
	tod->_dialog->redraw();
}

New_Tilemap_Dialog::New_Tilemap_Dialog(const char *t) : Option_Dialog(280, t), _tilemap_width(NULL), _tilemap_height(NULL),
	_format(NULL) {}

New_Tilemap_Dialog::~New_Tilemap_Dialog() {
	delete _tilemap_width;
	delete _tilemap_height;
	delete _format;
}

void New_Tilemap_Dialog::initialize_content() {
	// Populate content group
	_tilemap_width = new OS_Spinner(0, 0, 0, 0, "Width:");
	_tilemap_height = new OS_Spinner(0, 0, 0, 0, "Height:");
	_format = new Dropdown(0, 0, 0, 0, "Format:");
	// Initialize content group's children
	_tilemap_width->range(1, 1024);
	_tilemap_height->range(1, 1024);
	for (int i = 0; i < NUM_FORMATS; i++) {
		_format->add(format_name((Tilemap_Format)i));
	}
}

int New_Tilemap_Dialog::refresh_content(int ww, int dy) {
	int wgt_h = 22, win_m = 10, wgt_m = 4;
	int ch = wgt_h + wgt_m + wgt_h;
	_content->resize(win_m, dy, ww, ch);

	int wgt_off = win_m + MAX(text_width(_tilemap_width->label(), 2), text_width(_format->label(), 2));
	int wgt_w = text_width("9999", 2) + wgt_h / 2 + 4;
	_tilemap_width->resize(wgt_off, dy, wgt_w, wgt_h);
	int wgt_off2 = _tilemap_width->x() + _tilemap_width->w() + win_m + text_width("Height:", 2);
	_tilemap_height->resize(wgt_off2, dy, wgt_w, wgt_h);
	dy += wgt_h + wgt_m;
	wgt_w = format_max_name_width() + wgt_h;
	_format->resize(wgt_off, dy, wgt_w, wgt_h);

	return ch;
}

class Anchor_Button : public OS_Button {
private:
	int _anchor;
public:
	Anchor_Button(int a) : OS_Button(0, 0, 0, 0), _anchor(a) {
		type(FL_RADIO_BUTTON);
		align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	}
	inline int anchor(void) const { return _anchor; }
};

Resize_Dialog::Resize_Dialog(const char *t) : Option_Dialog(220, t), _tilemap_width(NULL), _tilemap_height(NULL), _anchor_buttons() {}

Resize_Dialog::~Resize_Dialog() {
	delete _tilemap_width;
	delete _tilemap_height;
	for (int i = 0; i < 9; i++) { delete _anchor_buttons[i]; }
}

Resize_Dialog::Hor_Align Resize_Dialog::horizontal_anchor() const {
	if (_anchor_buttons[0]->value() || _anchor_buttons[3]->value() || _anchor_buttons[6]->value()) { return Hor_Align::LEFT; }
	if (_anchor_buttons[2]->value() || _anchor_buttons[5]->value() || _anchor_buttons[8]->value()) { return Hor_Align::RIGHT; }
	return Hor_Align::CENTER;
}

Resize_Dialog::Vert_Align Resize_Dialog::vertical_anchor() const {
	if (_anchor_buttons[0]->value() || _anchor_buttons[1]->value() || _anchor_buttons[2]->value()) { return Vert_Align::TOP; }
	if (_anchor_buttons[6]->value() || _anchor_buttons[7]->value() || _anchor_buttons[8]->value()) { return Vert_Align::BOTTOM; }
	return Vert_Align::MIDDLE;
}

int Resize_Dialog::anchor() const {
	for (int i = 0; i < 9; i++) {
		if (_anchor_buttons[i]->value()) { return i; }
	}
	return 4;
}

void Resize_Dialog::anchor(int a) {
	_anchor_buttons[a]->do_callback();
}

void Resize_Dialog::anchor_label(int x, int y, const char *l) {
	if (0 <= x && x < 3 && 0 <= y && y < 3) {
		_anchor_buttons[y * 3 + x]->label(l);
	}
}

void Resize_Dialog::initialize_content() {
	// Populate content group
	_tilemap_width = new Default_Spinner(0, 0, 0, 0, "Width:");
	_tilemap_height = new Default_Spinner(0, 0, 0, 0, "Height:");
	for (int i = 0; i < 9; i++) {
		Anchor_Button *ab = new Anchor_Button(i);
		ab->callback((Fl_Callback *)anchor_button_cb, this);
		_anchor_buttons[i] = ab;
	}
	// Initialize content group's children
	_tilemap_width->range(1, 1024);
	_tilemap_height->range(1, 9999);
	anchor(Preferences::get("resize-anchor", 4));
}

int Resize_Dialog::refresh_content(int ww, int dy) {
	int wgt_w = 0, wgt_h = 22, win_m = 10, wgt_m = 4;
	int ch = (wgt_h + wgt_m) * 2 + wgt_h;
	_content->resize(win_m, dy, ww, ch);

	int wgt_off = win_m + MAX(text_width(_tilemap_width->label(), 2), text_width(_tilemap_height->label(), 2));

	wgt_w = text_width("9999", 2) + wgt_h / 2 + 4;
	_tilemap_width->resize(wgt_off, dy, wgt_w, wgt_h);
	_tilemap_height->resize(wgt_off, _tilemap_width->y() + _tilemap_width->h() + wgt_m, wgt_w, wgt_h);
	wgt_off += wgt_w + 20;

	wgt_w = wgt_h = 24;
	wgt_m = 2;
	for (int ay = 0; ay < 3; ay++) {
		for (int ax = 0; ax < 3; ax++) {
			_anchor_buttons[ay*3+ax]->resize(wgt_off + (wgt_w + wgt_m) * ax, dy, wgt_w, wgt_h);
		}
		dy += wgt_h + wgt_m;
	}

	return ch;
}

void Resize_Dialog::anchor_button_cb(Anchor_Button *ab, Resize_Dialog *rd) {
	ab->setonly();
	for (int i = 0; i < 9; i++) {
		rd->_anchor_buttons[i]->label(NULL);
	}
	int a = ab->anchor();
	int y = a / 3, x = a % 3;
	rd->anchor_label(x - 1, y - 1, "@7>"); // top-left
	rd->anchor_label(x,     y - 1, "@8>"); // top
	rd->anchor_label(x + 1, y - 1, "@9>"); // top-right
	rd->anchor_label(x - 1,  y,    "@4>"); // left
	rd->anchor_label(x,      y,    "@-2square"); // center
	rd->anchor_label(x + 1,  y,    "@>");  // right
	rd->anchor_label(x - 1, y + 1, "@1>"); // bottom-left
	rd->anchor_label(x,     y + 1, "@2>"); // bottom
	rd->anchor_label(x + 1, y + 1, "@3>"); // bottom-right
	rd->_dialog->redraw();
}

Reformat_Dialog::Reformat_Dialog(const char *t) : Option_Dialog(280, t), _format_header(NULL), _format(NULL), _force(NULL) {}

Reformat_Dialog::~Reformat_Dialog() {
	delete _format_header;
	delete _format;
	delete _force;
}

void Reformat_Dialog::format(Tilemap_Format fmt) {
	initialize();
	_format->value((int)fmt);
	const char *name = format_name(fmt);
	char buffer[256] = {};
	strcpy(buffer, "Convert ");
	strcat(buffer, name);
	strcat(buffer, " to:");
	_format_header->copy_label(buffer);
}

void Reformat_Dialog::initialize_content() {
	// Populate content group
	_format_header = new Label(0, 0, 0, 0);
	_format = new Dropdown(0, 0, 0, 0, "Format:");
	_force = new OS_Check_Button(0, 0, 0, 0, "Force (discards unformattable data)");
	// Initialize content group's children
	for (int i = 0; i < NUM_FORMATS; i++) {
		_format->add(format_name((Tilemap_Format)i));
	}
}

int Reformat_Dialog::refresh_content(int ww, int dy) {
	int wgt_h = 22, win_m = 10, wgt_m = 4;
	int ch = (wgt_h + wgt_m) * 2 + wgt_h;
	_content->resize(win_m, dy, ww, ch);

	_format_header->resize(win_m, dy, ww, wgt_h);
	dy += wgt_h + wgt_m;
	int wgt_off = win_m + text_width(_format->label(), 2);
	int wgt_w = format_max_name_width() + wgt_h;
	_format->resize(wgt_off, dy, wgt_w, wgt_h);
	dy += wgt_h + wgt_m;
	_force->resize(win_m, dy, ww, wgt_h);

	return ch;
}

Group_Width_Dialog::Group_Width_Dialog(const char *t) : Option_Dialog(194, t), _group_width(NULL) {}

Group_Width_Dialog::~Group_Width_Dialog() {
	delete _group_width;
}

void Group_Width_Dialog::initialize_content() {
	// Populate content group
	_group_width = new OS_Spinner(0, 0, 0, 0, "Width:");
	// Initialize content group's children
	_group_width->align(FL_ALIGN_LEFT);
	_group_width->range(1, 1024);
}

int Group_Width_Dialog::refresh_content(int ww, int dy) {
	int wgt_h = 22, win_m = 10;
	_content->resize(win_m, dy, ww, wgt_h);

	int wgt_off = win_m + text_width(_group_width->label(), 2);
	int wgt_w = text_width("9999", 2) + wgt_h / 2 + 4;
	_group_width->resize(wgt_off, dy, wgt_w, wgt_h);

	return wgt_h;
}

Add_Tileset_Dialog::Add_Tileset_Dialog(const char *t) : Option_Dialog(270, t), _tileset_header(NULL), _start_id(NULL),
	_offset(NULL), _length(NULL) {}

Add_Tileset_Dialog::~Add_Tileset_Dialog() {
	delete _tileset_header;
	delete _start_id;
	delete _offset;
	delete _length;
}

void Add_Tileset_Dialog::limit_tileset_options(const char *filename) {
	initialize();
	const char *name = fl_filename_name(filename);
	char buffer[FL_PATH_MAX] = {};
	strcpy(buffer, name);
	strcat(buffer, ":");
	_tileset_header->copy_label(buffer);
}

void Add_Tileset_Dialog::initialize_content() {
	// Populate content group
	_tileset_header = new Label(0, 0, 0, 0);
	_start_id = new Default_Hex_Spinner(0, 0, 0, 0, "Start at ID: $");
	_offset = new Default_Spinner(0, 0, 0, 0, "Offset:");
	_length = new Default_Spinner(0, 0, 0, 0, "Length:");
	// Initialize content group's children
	_start_id->format("%03X");
	_start_id->range(0x00, MAX_NUM_TILES-1);
	_start_id->default_value(0x00);
	_offset->align(FL_ALIGN_LEFT);
	_offset->range(0, 1024);
	_offset->default_value(0);
	_length->align(FL_ALIGN_LEFT);
	_length->range(0, 1024);
	_length->default_value(0);
}

int Add_Tileset_Dialog::refresh_content(int ww, int dy) {
	int wgt_h = 22, win_m = 10, wgt_m = 4;
	int ch = (wgt_h + wgt_m) * 2 + wgt_h;
	_content->resize(win_m, dy, ww, ch);

	_tileset_header->resize(win_m, dy, ww, wgt_h);
	dy += wgt_h + wgt_m;
	int wgt_off = win_m + text_width(_start_id->label(), 3);
	int wgt_w = MAX(text_width("AAA", 2), text_width("FFF", 2)) + wgt_h / 2 + 4;
	_start_id->resize(wgt_off, dy, wgt_w, wgt_h);
	dy += wgt_h + wgt_m;
	wgt_off = win_m + text_width(_offset->label(), 3);
	wgt_w = text_width("9999", 2) + wgt_h / 2 + 4;
	_offset->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off = _offset->x() + _offset->w() + win_m + text_width(_length->label(), 3);
	_length->resize(wgt_off, dy, wgt_w, wgt_h);

	return ch;
}

Image_To_Tiles_Dialog::Image_To_Tiles_Dialog(const char *t) : Option_Dialog(360, t), _input_heading(NULL),
	_output_heading(NULL), _input_spacer(NULL), _output_spacer(NULL), _image_heading(NULL), _tileset_heading(NULL),
	_image(NULL), _tileset(NULL), _image_name(NULL), _tileset_name(NULL), _format(NULL), _output_names(NULL),
	_palette(NULL), _palette_format(NULL), _palette_name(NULL), _start_id(NULL), _use_space(NULL), _space_id(NULL),
	_image_chooser(NULL), _tileset_chooser(NULL), _image_filename(), _tileset_filename(), _tilemap_filename(),
	_attrmap_filename(), _palette_filename(), _tilepal_filename() {}

Image_To_Tiles_Dialog::~Image_To_Tiles_Dialog() {
	delete _input_heading;
	delete _output_heading;
	delete _input_spacer;
	delete _output_spacer;
	delete _image_heading;
	delete _tileset_heading;
	delete _image;
	delete _tileset;
	delete _image_name;
	delete _tileset_name;
	delete _format;
	delete _output_names;
	delete _palette;
	delete _palette_format;
	delete _palette_name;
	delete _start_id;
	delete _use_space;
	delete _space_id;
	delete _image_chooser;
	delete _tileset_chooser;
}

void Image_To_Tiles_Dialog::update_image_name() {
	if (_image_filename.empty()) {
		_image_name->label(NO_FILE_SELECTED_LABEL);
	}
	else {
		const char *basename = fl_filename_name(image_filename());
		_image_name->copy_label(basename);
	}
}

static const char *palette_names[NUM_PALETTE_FORMATS] = {"RGB", "JASC", "ACT", "GPL", "PNG", "BMP"};

static const char *palette_exts[NUM_PALETTE_FORMATS] = {".pal", ".pal", ".act", ".gpl", ".pal.png", ".pal.bmp"};

void Image_To_Tiles_Dialog::update_output_names() {
	if (_tileset_filename.empty()) {
		_tileset_name->label(NO_FILE_SELECTED_LABEL);
		_tilemap_filename.clear();
		_attrmap_filename.clear();
		_palette_filename.clear();
		_tilepal_filename.clear();
		_output_names->label("Tilemap: " NO_FILE_SELECTED_LABEL);
	}
	else {
		_tileset_name->copy_label(fl_filename_name(tileset_filename()));

		char output_filename[FL_PATH_MAX] = {};
		strcpy(output_filename, tileset_filename());
		fl_filename_setext(output_filename, sizeof(output_filename), format_extension(format()));
		_tilemap_filename = output_filename;

		strcpy(output_filename, tileset_filename());
		fl_filename_setext(output_filename, sizeof(output_filename), ATTRMAP_EXT);
		_attrmap_filename = output_filename;

		strcpy(output_filename, tileset_filename());
		const char *palette_ext = palette_exts[(int)palette_format()];
		fl_filename_setext(output_filename, sizeof(output_filename), palette_ext);
		_palette_filename = output_filename;

		strcpy(output_filename, tileset_filename());
		fl_filename_setext(output_filename, sizeof(output_filename), TILEPAL_EXT);
		_tilepal_filename = output_filename;

		char output_names[FL_PATH_MAX] = {};
		strcpy(output_names, "Tilemap: ");
		strcat(output_names, fl_filename_name(tilemap_filename()));
		if (format_has_attrmap(format())) {
			strcat(output_names, " / Attrmap: ");
			strcat(output_names, fl_filename_name(attrmap_filename()));
		}
		_output_names->copy_label(output_names);
	}

	if (format_can_make_palettes(format())) {
		_palette->activate();
		_palette_format->activate();
		_palette_name->activate();
		if (_palette_filename.empty()) {
			_palette_name->label(NO_FILE_SELECTED_LABEL);
		}
		else {
			char output_names[FL_PATH_MAX] = {};
			strcpy(output_names, fl_filename_name(palette_filename()));
			if (format_has_per_tile_palettes(format())) {
				strcat(output_names, " / ");
				strcat(output_names, fl_filename_name(tilepal_filename()));
			}
			_palette_name->copy_label(output_names);
		}
	}
	else {
		_palette->clear();
		_palette_format->value(0);
		_palette_name->label("N/A");
		_palette->deactivate();
		_palette_format->deactivate();
		_palette_name->deactivate();
	}
}

void Image_To_Tiles_Dialog::update_ok_button() {
	if (_image_filename.empty() || _tileset_filename.empty()) {
		_ok_button->deactivate();
	}
	else {
		_ok_button->activate();
	}
}

Image_To_Tiles_Dialog::Palette_Format Image_To_Tiles_Dialog::default_palette_format(Tilemap_Format fmt) const {
	switch (fmt) {
	case Tilemap_Format::GBA_4BPP:
	case Tilemap_Format::GBA_8BPP:
		return Palette_Format::JASC;
	case Tilemap_Format::GBC_ATTRS:
	case Tilemap_Format::GBC_ATTRMAP:
	case Tilemap_Format::SGB_BORDER:
	case Tilemap_Format::RBY_TOWN_MAP:
	case Tilemap_Format::GSC_TOWN_MAP:
	case Tilemap_Format::PC_TOWN_MAP:
	case Tilemap_Format::POKEGEAR_CARD:
		return Palette_Format::RGB;
	case Tilemap_Format::PLAIN:
	case Tilemap_Format::SNES_ATTRS:
	default:
		return palette_format();
	}
}

void Image_To_Tiles_Dialog::initialize_content() {
	// Populate content group
	_input_heading = new Label(0, 0, 0, 0, "Input:");
	_input_spacer = new Spacer(0, 0, 0, 0);
	_image_heading = new Label(0, 0, 0, 0, "Image:");
	_output_heading = new Label(0, 0, 0, 0, "Output:");
	_output_spacer = new Spacer(0, 0, 0, 0);
	_tileset_heading = new Label(0, 0, 0, 0, "Tileset:");
	_image = new Toolbar_Button(0, 0, 0, 0);
	_tileset = new Toolbar_Button(0, 0, 0, 0);
	_image_name = new Label_Button(0, 0, 0, 0, NO_FILE_SELECTED_LABEL);
	_tileset_name = new Label_Button(0, 0, 0, 0, NO_FILE_SELECTED_LABEL);
	_format = new Dropdown(0, 0, 0, 0, "Format:");
	_output_names = new Label(0, 0, 0, 0, "Tilemap: " NO_FILE_SELECTED_LABEL);
	_palette = new OS_Check_Button(0, 0, 0, 0, "Palette:");
	_palette_format = new Dropdown(0, 0, 0, 0);
	_palette_name = new Label(0, 0, 0, 0, NO_FILE_SELECTED_LABEL);
	_start_id = new Default_Hex_Spinner(0, 0, 0, 0, "Start at ID: $");
	_use_space = new OS_Check_Button(0, 0, 0, 0, "Blank Spaces Use ID: $");
	_space_id = new Default_Hex_Spinner(0, 0, 0, 0);
	_image_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_FILE);
	_tileset_chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	// Initialize content group's children
	_image->callback((Fl_Callback *)image_cb, this);
	_image->image(INPUT_ICON);
	_tileset->callback((Fl_Callback *)tileset_cb, this);
	_tileset->image(OUTPUT_ICON);
	_image_name->callback((Fl_Callback *)image_cb, this);
	_tileset_name->callback((Fl_Callback *)tileset_cb, this);
	for (int i = 0; i < NUM_FORMATS; i++) {
		_format->add(format_name((Tilemap_Format)i));
	}
	_format->callback((Fl_Callback *)format_cb, this);
	for (int i = 0; i < NUM_PALETTE_FORMATS; i++) {
		_palette_format->add(palette_names[i]);
	}
	_palette_format->value(0);
	_palette_format->callback((Fl_Callback *)palette_format_cb, this);
	_start_id->format("%03X");
	_start_id->range(0x000, MAX_NUM_TILES-1);
	_start_id->default_value(0x000);
	_use_space->callback((Fl_Callback *)use_space_cb, this);
	_space_id->format("%03X");
	_space_id->range(0x000, MAX_NUM_TILES-1);
	_space_id->default_value(0x07F);
	_image_chooser->title("Read Image");
	_image_chooser->filter("Image Files\t*.{png,bmp}\n");
	_tileset_chooser->title("Write Tileset");
	_tileset_chooser->filter("PNG Files\t*.png\nBMP Files\t*.bmp\n");
	_tileset_chooser->options(Fl_Native_File_Chooser::Option::SAVEAS_CONFIRM);
}

int Image_To_Tiles_Dialog::refresh_content(int ww, int dy) {
	int wgt_h = 22, win_m = 10, wgt_m = 4;
	int ch = (wgt_h + wgt_m) * 7 + wgt_h;
	_content->resize(win_m, dy, ww, ch);

	int wgt_w = text_width(_input_heading->label(), 4);
	int wgt_off = win_m;

	_input_heading->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off += _input_heading->w();
	_input_spacer->resize(wgt_off, dy+wgt_h/2-1, ww-wgt_w, 2);
	dy += wgt_h + wgt_m;

	int wgt_w2 = MAX(text_width(_image_heading->label(), 4), text_width(_tileset_heading->label(), 4));
	wgt_off = win_m;

	_image_heading->resize(wgt_off, dy, wgt_w2, wgt_h);
	wgt_off += _image_heading->w();
	_image->resize(wgt_off, dy, wgt_h, wgt_h);
	wgt_off += _image->w();
	_image_name->resize(wgt_off, dy, ww-wgt_w2-wgt_h, wgt_h);
	dy += wgt_h + wgt_m;

	wgt_w = text_width(_output_heading->label(), 4);
	wgt_off = win_m;

	_output_heading->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off += _output_heading->w();
	_output_spacer->resize(wgt_off, dy+wgt_h/2-1, ww-wgt_w, 2);
	dy += wgt_h + wgt_m;

	wgt_off = win_m;

	_tileset_heading->resize(wgt_off, dy, wgt_w2, wgt_h);
	wgt_off += _tileset_heading->w();
	_tileset->resize(wgt_off, dy, wgt_h, wgt_h);
	wgt_off += _tileset->w();
	_tileset_name->resize(wgt_off, dy, ww-wgt_w2-wgt_h, wgt_h);
	dy += wgt_h + wgt_m;

	wgt_off = win_m + text_width(_format->label(), 3);
	wgt_w = format_max_name_width() + wgt_h;
	_format->resize(wgt_off, dy, wgt_w, wgt_h);
	dy += wgt_h + wgt_m;

	_output_names->resize(win_m, dy, ww, wgt_h);
	dy += wgt_h + wgt_m;

	wgt_w = _palette->labelsize() + 4 + text_width(_palette->label());
	wgt_off = win_m;
	_palette->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off += _palette->w() + 4;
	wgt_w = text_width("JASC", 6) + wgt_h;
	_palette_format->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off += _palette_format->w() + wgt_m;
	wgt_w = ww - wgt_off + win_m;
	_palette_name->resize(wgt_off, dy, wgt_w, wgt_h);
	dy += wgt_h + wgt_m;

	wgt_w = MAX(text_width("AAA", 2), text_width("FFF", 2)) + wgt_h / 2 + 4;
	wgt_off = win_m + text_width(_start_id->label(), 3);
	_start_id->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_w = text_width(_use_space->label(), 3) + _use_space->labelsize() + 4;
	wgt_off += _start_id->w() + win_m;
	_use_space->resize(wgt_off, dy, wgt_w, wgt_h);
	wgt_off += _use_space->w();
	wgt_w = MAX(text_width("AAA", 2), text_width("FFF", 2)) + wgt_h / 2 + 4;
	_space_id->resize(wgt_off, dy, wgt_w, wgt_h);

	_image_filename.clear();
	_tileset_filename.clear();
	update_image_name();
	update_output_names();
	update_ok_button();

	use_space_cb(_use_space, this);

	return ch;
}

void Image_To_Tiles_Dialog::image_cb(Fl_Widget *, Image_To_Tiles_Dialog *itd) {
	int status = itd->_image_chooser->show();
	if (status == 1) {
		itd->_image_filename.clear();
		itd->_image_name->label(NO_FILE_SELECTED_LABEL);
	}
	else {
		const char *filename = itd->_image_chooser->filename();
		itd->_image_filename = filename;
		const char *basename = fl_filename_name(filename);
		itd->_image_name->copy_label(basename);
	}
	itd->update_ok_button();
	itd->_dialog->redraw();
}

void Image_To_Tiles_Dialog::tileset_cb(Fl_Widget *, Image_To_Tiles_Dialog *itd) {
	int status = itd->_tileset_chooser->show();
	if (status == 1) {
		itd->_tileset_filename.clear();
	}
	else {
		char filename[FL_PATH_MAX] = {};
		const char *default_ext = itd->_tileset_chooser->filter_value() == 1 ? ".bmp" : ".png";
		add_dot_ext(itd->_tileset_chooser->filename(), default_ext, filename);
		itd->_tileset_filename.assign(filename);
	}
	itd->update_output_names();
	itd->update_ok_button();
	itd->_dialog->redraw();
}

void Image_To_Tiles_Dialog::format_cb(Dropdown *, Image_To_Tiles_Dialog *itd) {
	Palette_Format pal_fmt = itd->default_palette_format(itd->format());
	itd->_palette_format->value((int)pal_fmt);
	itd->update_output_names();
}

void Image_To_Tiles_Dialog::palette_format_cb(Dropdown *, Image_To_Tiles_Dialog *itd) {
	itd->update_output_names();
}

void Image_To_Tiles_Dialog::use_space_cb(OS_Check_Button *, Image_To_Tiles_Dialog *itd) {
	if (itd->use_space()) {
		itd->_space_id->activate();
	}
	else {
		itd->_space_id->deactivate();
	}
	itd->_space_id->redraw();
}
