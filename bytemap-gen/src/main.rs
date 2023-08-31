use std::io::Write;

use noto_sans_mono_bitmap::{get_raster, get_raster_width, FontWeight, RasterHeight};

// Minimal example.
fn main() {
    let rHeight = RasterHeight::Size16;
    let width = get_raster_width(FontWeight::Regular, rHeight);

    for i in b' '..=b'~' {
        let char_raster = get_raster(i as char, FontWeight::Regular, rHeight)
            .expect(&format!("unsupported char {}", i as char));

        for row in char_raster.raster() {
            std::io::stdout().write_all(row).expect("failed to write");
        }
    }
    eprintln!(
        "Printed {} characters, width {}, height {}",
        b'~' + 1 - b' ',
        width,
        rHeight.val()
    );
}
