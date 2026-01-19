SECTIONS {
	. = 0xC0000;
	.text : ALIGN(16) {*(.text)}
	.rodata : ALIGN(16) {*(.rodata*)}
	.data : ALIGN(16) {*(.data)}
}
