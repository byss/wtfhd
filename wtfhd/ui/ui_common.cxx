//
//  ui_common.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#include "ui_common.hxx"

#include "screen.hxx"

int ui::main () {
	return screen::run_main ();
}

void ui::exit (int rc) {
	screen::exit (rc);
}
