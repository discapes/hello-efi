use anyhow::Result;
use std::io::Seek;
use std::io::SeekFrom;
use std::process::Command;
use std::{fs::File, io::Write};

use noto_sans_mono_bitmap::{get_raster, get_raster_width, FontWeight, RasterHeight};

// Minimal example.
fn render_height(r_height: RasterHeight) -> Result<()> {
    let filename = format!("noto_{}.map", r_height.val());
    let cfilename = format!("noto_{}.h", r_height.val());
    let mut file = File::create(&filename)?;

    let width = get_raster_width(FontWeight::Regular, r_height);

    for i in b' '..=b'~' {
        let char_raster = get_raster(i as char, FontWeight::Regular, r_height)
            .expect(&format!("unsupported char {}", i as char));

        for row in char_raster.raster() {
            file.write_all(row).expect("failed to write");
        }
    }

    let pos = file.seek(SeekFrom::Current(0))?;
    eprintln!(
        "Printed {} characters into {}, width {}, height {}, wrote {:.2} KiB",
        b'~' + 1 - b' ',
        filename,
        width,
        r_height.val(),
        pos as f64 / 1024.
    );

    Command::new("xxd")
        .arg("-i")
        .arg("-c")
        .arg(width.to_string())
        .arg(filename)
        .arg(cfilename)
        .spawn()
        .expect("xxd failed");

    Ok(())
}

fn main() -> Result<()> {
    render_height(RasterHeight::Size16)?;
    render_height(RasterHeight::Size20)?;
    render_height(RasterHeight::Size24)?;
    render_height(RasterHeight::Size32)?;
    Ok(())
}
