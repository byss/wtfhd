//
//  ui_common.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef ui_common_hxx
#define ui_common_hxx

#include <chrono>

namespace ui {
	struct rect;
	
	int main ();
	void exit (int);
}

struct ui::rect {
	int x, y, width, height;
	
	static rect centered (rect parent, int width, int height) {
		return rect ((parent.width - width) / 2, (parent.height - height) / 2, width, height);
	}
	
	rect (): rect (0, 0, 0, 0) {}
	rect (int x, int y, int width, int height): x (x), y (y), width (width), height (height) {}
	~rect () = default;
	
	rect inset (int d) const {
		return this->inset (d, d);
	}
	
	rect inset (int dx, int dy) const {
		return rect (this->x + dx, this->y + dy, this->width - 2 * dx, this->height - 2 * dy);
	}
};

#endif /* ui_common_hxx */
