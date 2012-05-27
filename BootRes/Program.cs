// BootRes: Breaks apart and create the activity.bmp file in bootres.dll
// Copyright (C) 2010  Jeffrey Bush <jeff@coderforlife.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.

using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;

namespace BootRes
{
    class Program
    {
        const int ANIM_WIDTH  = 200;
        const int ANIM_HEIGHT = 200;
        const int FRAMES      = 105;

        const int HEADER_SIZE = 54;

        const long creation_time     = 0x01C9EA0A584DA403; //6/10/2009 1:30:50 PM
        const long modification_time = 0x01C9EA0A586EF727; //6/10/2009 1:30:51 PM
        const long accessed_time     = 0x01C9EA0A584DA403; //6/10/2009 1:30:50 PM

        static int Main(string[] args)
        {
            Console.WriteLine("BootRes Copyright (C) 2010-2012  Jeffrey Bush <jeff@coderforlife.com>");
            Console.WriteLine("This program comes with ABSOLUTELY NO WARRANTY;");
            Console.WriteLine("This is free software, and you are welcome to redistribute it");
            Console.WriteLine("under certain conditions;");
            Console.WriteLine("See http://www.gnu.org/licenses/gpl.html for more details.");
            Console.WriteLine();

            if (args.Length != 2 && args.Length != 3)
            {
                Console.WriteLine("Usage: BOOTRES src dest [BMP|PNG|GIF|TIF]");
                Console.WriteLine("If src is a file and the dest is a directory, it will break the file up");
                Console.WriteLine("If src is a directory and the dest is a file, it will piece the files together");
                Console.WriteLine("The last parameter is for file type of pieces, defaults to BMP");
                return 1;
            }

            string src = args[0];
            string dest = args[1];
            ImageFormat format = ImageFormat.Bmp;
            if (args.Length == 3)
            {
                string f = args[2].ToUpper();
                if (f == "PNG")
                {
                    format = ImageFormat.Png;
                }
                else if (f == "GIF")
                {
                    format = ImageFormat.Png;
                }
                else if (f == "TIF")
                {
                    format = ImageFormat.Tiff;
                }
                else if (f != "BMP")
                {
                    Console.Error.WriteLine("The format for pieces was not understood, defaulting to BMP");
                }
            }
    
            if (Directory.Exists(src))
            {
                if (Directory.Exists(dest))
                {
                    Console.Error.WriteLine("Source and destination cannot both be directories.");
                    return 1;
                }
                return PieceTogether(src, dest, format) ? 0 : 1;
            }
            else if (File.Exists(src))
            {
                if (File.Exists(dest))
                {
                    Console.Error.WriteLine("Source and destination cannot both be files.");
                    return 1;
                }
                return BreakUp(src, dest, format) ? 0 : 1;
            }
            else
            {
                Console.Error.WriteLine("The source does not exist.");
                return 1;
            }
        }

        static void SetInt(int x, byte[] arr, int offset)
        {
            for (int i = 0; i < 4; i++)
            {
                arr[offset+i] = (byte)((x >> (i * 8)) & 0xFF);
            }
        }

        static Bitmap GetBitmap(string path, bool displayError)
        {
            Bitmap x = null;
            try
            {
                x = new Bitmap(path);
                if (x.Width != ANIM_WIDTH || x.Height != ANIM_HEIGHT)
                {
                    if (displayError)
                    {
                        Console.Error.WriteLine("The file '"+path+"' is the wrong size (needs to be "+ANIM_WIDTH+"x"+ANIM_HEIGHT+")");
                    }
                    x = null;
                }
            }
            catch (Exception ex)
            {
                if (displayError)
                {
                    Console.Error.WriteLine("There was a problem opening the file '"+path+"': "+ex.Message);
                }
            }
            return x;
        }

        static string GetExt(ImageFormat format)
        {
            if (format == ImageFormat.Png)
                return ".png";
            else if (format == ImageFormat.Gif)
                return ".gif";
            else if (format == ImageFormat.Tiff)
                return ".tif";
            else
                return ".bmp";
        }

        static bool BreakUp(String src, String dest, ImageFormat format)
        {
            Bitmap b = null;
            try {
                b = new Bitmap(src);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("There was a problem opening the source image '"+src+"': "+ex.Message);
                return false;
            }
            if (b.Width != ANIM_WIDTH || b.Height % ANIM_HEIGHT != 0)
            {
                Console.Error.WriteLine("The source image must be "+ANIM_WIDTH+"px wide and a multiple of "+ANIM_HEIGHT+"px tall");
                return false;
            }
            if (!Directory.Exists(dest))
            {
                try {
                    Directory.CreateDirectory(dest);
                } catch (Exception ex) {
                    Console.Error.WriteLine("There was a problem creating the directory '"+dest+"': "+ex.Message);
                    return false;
                }
            }
            string name = Path.GetFileNameWithoutExtension(src);
            int frames = b.Height / ANIM_HEIGHT;
            Rectangle r = new Rectangle(0, 0, ANIM_WIDTH, ANIM_HEIGHT);
            for (int i = 0; i < frames; i++)
            {
                r.Y = i*ANIM_HEIGHT;
                string path = Path.Combine(dest, name+i.ToString("000")+GetExt(format));
                Bitmap x = b.Clone(r, PixelFormat.Format24bppRgb);
                try
                {
                    x.Save(path, format);
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine("Unable to save '"+path+"': "+ex.Message);
                }
            }
            return true;
        }

        static bool PieceTogether(string src, string dest, ImageFormat format)
        {
            // Open the directory
            DirectoryInfo dir = null;
            try
            {
                dir = new DirectoryInfo(src);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("There was a problem opening the directory '"+src+"': "+ex.Message);
                return false;
            }

            // Go through each file
            System.Collections.Generic.List<string> files = new System.Collections.Generic.List<string>();
            foreach (FileInfo fi in dir.GetFiles("*"+GetExt(format)))
            {
                if (GetBitmap(fi.FullName, true) != null)
                {
                    files.Add(fi.FullName);
                }
            }
            if (files.Count == 0)
            {
                Console.Error.WriteLine("There were no acceptable files found in '"+src+"'");
                return false;
            }
            if (files.Count != FRAMES)
            {
                Console.Error.WriteLine("Warning: Expected "+FRAMES+" frames but found "+files.Count+" frames in '"+src+"'");
            }
    
            files.Sort();
    
            int size = ANIM_WIDTH * files.Count*ANIM_HEIGHT * 3; // bitmap data size
            Bitmap b = new Bitmap(ANIM_WIDTH, files.Count*ANIM_HEIGHT, PixelFormat.Format24bppRgb);
            Graphics g = Graphics.FromImage(b);
            for (int i = 0; i < files.Count; i++)
            {
                g.DrawImageUnscaled(GetBitmap(files[i], false), 0, i*ANIM_HEIGHT);
            }

            try
            {
                b.Save(dest, ImageFormat.Bmp);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("There was a problem saving to '"+dest+"': "+ex.Message);
                return false;
            }

            try
            {
                FileStream s = File.Open(dest, FileMode.Open, FileAccess.ReadWrite);
                byte[] x = new byte[HEADER_SIZE];
                s.Seek(0, SeekOrigin.Begin);
                s.Read(x, 0, HEADER_SIZE);
                //Adjust file size to be +1 of real (this is how the real activity.bmp is)
                SetInt(size+HEADER_SIZE+1, x, 0x02);
                //Add the BMP data size
                SetInt(size, x, 0x22);
                //Set the horiz and vert resolution to 0
                SetInt(0, x, 0x26);
                SetInt(0, x, 0x2A);
                s.Seek(0, SeekOrigin.Begin);
                s.Write(x, 0, HEADER_SIZE);
                s.Close();
                File.SetCreationTimeUtc(dest, DateTime.FromFileTime(creation_time));
                File.SetLastWriteTimeUtc(dest, DateTime.FromFileTime(modification_time));
                File.SetLastAccessTimeUtc(dest, DateTime.FromFileTime(accessed_time));
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("There was a problem adjusting the saved document '"+dest+"': "+ex.Message);
                return false;
            }

            return true;
        }

    }
}
