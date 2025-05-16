/*!
A binary program that displays rendering results, with a movable camera.
*/
#[warn(missing_docs)]
mod camera;

slint::include_modules!();

fn main() -> Result<(), slint::PlatformError> {
   let main_window = MainWindow::new()?;

   main_window.run()
}
