using System;
using System.IO;
using System.Drawing;

namespace FirstUXRes
{
    // Simple structure for holding color values as 4 doubles, each from 0.0 to 1.0
    struct RGBA
    {
        public double R, G, B, A;

        public RGBA(double R, double G, double B, double A = 1.0) { this.R = R; this.G = G; this.B = B; this.A = A; }

        // Converts RGBA to System::Drawing::Color
        public Color ToColor() { return Color.FromArgb((int)(this.A * 255), (int)(this.R * 255), (int)(this.G * 255), (int)(this.B * 255)); }
        public static explicit operator Color(RGBA rgba) { return rgba.ToColor(); }

        // Converts System::Drawing::Color to RGBA
        public RGBA(Color c) : this(c.R / 255.0, c.G / 255.0, c.B / 255.0, c.A / 255.0) { }
        public static explicit operator RGBA(Color c) { return new RGBA(c); }


        //This algorithm for converting a color to alpha is based on the one used by GIMP
        //http://git.gnome.org/browse/gimp/tree/plug-ins/common/color-to-alpha.c

        private static double ChannelToAlpha(double src, double color)
        {
            if (color < 0.0001)     return src;
            else if (src > color)   return (src - color) / (1.0 - color);
            else if (src < color)   return (color - src) / color;
            else                    return 0.0;
        }

        public RGBA ColorToAlpha(RGBA color) // this is color from image, color is color to replace with alpha
        {
            RGBA x = new RGBA(
                ChannelToAlpha(this.R, color.R),
                ChannelToAlpha(this.G, color.G),
                ChannelToAlpha(this.B, color.B),
                this.A
            );
            if (x.R > x.G) this.A = (x.R > x.B) ? x.R : x.B;
            else	       this.A = (x.G > x.B) ? x.G : x.B;
            if (this.A >= 0.0001)
            {
                this.R = (this.R - color.R) / this.A + color.R;
                this.G = (this.G - color.G) / this.A + color.G;
                this.B = (this.B - color.B) / this.A + color.B;
                this.A *= x.A;
            }
            return this;
        }

        // Mixes two colors based on their alpha value, placing this over the other color.
        // Note: does not mix alpha values, simply sets the alpha to 1.0
        public RGBA Mix(RGBA other)
        {
            this.R = this.R * this.A + other.R * other.A * (1.0 - this.A);
            this.G = this.G * this.A + other.G * other.A * (1.0 - this.A);
            this.B = this.B * this.A + other.B * other.A * (1.0 - this.A);
            this.A = 1.0;
            return this;
        }

    };

    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length != 3)
            {
                Console.WriteLine("You need to include the background image (PNG, GIF, JPEG, EXIG, or TIFF) file, a source directory (containing all the FirstUXRes animation files), and a destination directory on the command line.");
                return 1;
            }

            // Color to replace
            RGBA black = new RGBA(0.0, 0.0, 0.0, 1.0);

            // Open the background image
            Bitmap bkgrnd = null;
            try
            {
                bkgrnd = new Bitmap(args[0]);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("There was a problem opening the background image '"+args[0]+"': "+ex.Message);
                return 1;
            }

            // Target height and width
            int width = bkgrnd.Width, height = bkgrnd.Height;

            // Open the directory
            DirectoryInfo dir = null;
            try
            {
                dir = new DirectoryInfo(args[1]);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("There was a problem opening the directory '"+args[1]+"': "+ex.Message);
                return 1;
            }

            // Create/open destination directory
            String dest = args[2];
            if (File.Exists(dest))
            {
                Console.Error.WriteLine("The path '"+dest+"' is not a directory");
                return 1;
            }
            else if (!Directory.Exists(dest))
            {
                try
                {
                    Directory.CreateDirectory(dest);
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine("There was a problem creating the directory '"+dest+"': "+ex.Message);
                    return 1;
                }
            }

            // Go through each file
            foreach (FileInfo fi in dir.GetFiles("*.bmp"))
            {
                String name = fi.FullName;
                String file = Path.GetFileName(name);
                try {
                    // Open it
                    Bitmap img = new Bitmap(name);
                    if (img.Width != width || img.Height != height) {
                        Console.WriteLine("Skipping '"+name+"' because it was the wrong size.");
                        continue;
                    }
                    // Go through each pixel
                    for (int x = 0; x < width; x++) {
                        for (int y = 0; y < height; y++) {
                            // Get the foreground and background colors
                            RGBA f = (RGBA)img.GetPixel(x, y);
                            RGBA b = (RGBA)bkgrnd.GetPixel(x, y);
                            // Convert to alpha, mix, and replace pixel
                            img.SetPixel(x, y, (Color)f.ColorToAlpha(black).Mix(b));
                        }
                    }
                    img.Save(dest+"\\"+file, System.Drawing.Imaging.ImageFormat.Bmp);
                } catch (Exception ex) {
                    Console.Error.WriteLine("There was a problem editing the image '"+file+"': "+ex.Message);
                }
            }
    
            return 0;
        }
    }
}
