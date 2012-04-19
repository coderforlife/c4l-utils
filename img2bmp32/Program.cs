using System;
using System.IO;
using System.Drawing;

namespace System.Runtime.CompilerServices { public class ExtensionAttribute : Attribute { } }

namespace img2bmp32
{
    static class Program
    {
        static int Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("You need to include at least one GIF, JPEG, PNG, TIFF, WMF, EMF, BMP, or ICO file on the command line");
            }
            for (int i = 0; i < args.Length; i++)
            {
                Bitmap img = null;
                FileStream bmp = null;
                try
                {
                    // Open the source image
                    img = new Bitmap(args[i]);

                    // Get the name of the new file (replace extension with .bmp)
                    String name = args[i];
                    int dot = name.LastIndexOf(".");
                    if (dot > 0)
                        name = name.Remove(dot);
                    name += ".bmp";

                    // Calculate the size of the bitmap data in the image
                    int w = img.Width, h = img.Height, bmpSize = w * h * 4;

                    // Open the BMP file
                    bmp = new FileStream(name, FileMode.Create, FileAccess.Write);

                    // Write BMP Magic Number
                    bmp.WriteByte((byte)'B');
                    bmp.WriteByte((byte)'M');

                    // Write BMP File Header
                    bmp.WriteLong(54+bmpSize);  // Size of the entire file
                    bmp.WriteLong(0);           // Reserved WORDs 1 and 2
                    bmp.WriteLong(54);          // Bitmap data start (always 54)

                    // Write Bitmap data header
                    bmp.WriteLong(40);      // Header size (40 for Windows V3 / BITMAPINFOHEADER)
                    bmp.WriteLong(w);       // Image width
                    bmp.WriteLong(h);       // Image height
                    bmp.WriteShort(1);      // Number of color planes (must be 1)
                    bmp.WriteShort(32);     // Bits per pixel (32 for full color and alpha images)
                    bmp.WriteLong(0);       // Compression type (0 for no compression)
                    bmp.WriteLong(bmpSize); // Size of the bitmap data
                    bmp.WriteLong(2835);    // Horizontal resolution of image (2835 px/m is typical)
                    bmp.WriteLong(2835);    // Vertical resolution of image (2835 px/m is typical)
                    bmp.WriteLong(0);       // Number of colors in palette (0 for no palette)
                    bmp.WriteLong(0);       // Number of important colors in palette (0 for all colors are important)

                    // Write bitmap data
                    bmp.Seek(54, SeekOrigin.Begin);
                    for (int y = h - 1; y >= 0; y--)
                    {
                        for (int x = 0; x < w; x++)
                        {
                            Color c = img.GetPixel(x, y);
                            byte[] px = { c.B, c.G, c.R, c.A }; // BMP file is backwards (BGRA instead of ARGB)
                            px[0] = (byte)(px[0] * px[3] / 255); // We must pre-multiply the alpha values
                            px[1] = (byte)(px[1] * px[3] / 255);
                            px[2] = (byte)(px[2] * px[3] / 255);
                            bmp.Write(px, 0, 4); // Write the pixel
                        }
                    }
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine("There was a problem converting " + args[i] + ": " + ex.Message);
                }
                finally
                {
                    if (bmp != null) bmp.Dispose();
                    if (img != null) img.Dispose();
                }
            }
    
            return 0;
        }

        public static void WriteShort(this FileStream f, short x) { f.Write(System.BitConverter.GetBytes(x), 0, sizeof(short)); }
        public static void WriteLong(this FileStream f, long x) { f.Write(System.BitConverter.GetBytes(x), 0, sizeof(long)); }
    }
}
