using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace multitext
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length >= 3)
            {
                uint x;
                     if (CheckArgument(args[0], "ADD",     1) && uint.TryParse(args[1], out x)) { return AddLine   (--x, GetFiles(args, 2)); } // need to change from 1 to 0 based
                else if (CheckArgument(args[0], "DELETE",  1) && uint.TryParse(args[1], out x)) { return DeleteLine(--x, GetFiles(args, 2)); } // need to change from 1 to 0 based
                else if (CheckArgument(args[0], "COMPARE", 1))
                {
                    bool equal = args.Length >= 4 && CheckArgument(args[1], "EQUAL", 1);
                    int index = equal ? 2 : 1;
                    x = 0;
                    if (args.Length >= (index + 3) && uint.TryParse(args[index], out x))
                        ++index;
                    return CompareFiles(equal, --x, args[index], args[index + 1]);
                }
            }
            Usage();
            return -1;
        }

        static void Usage()
        {
            Console.WriteLine("Usage:");
            Console.WriteLine("  multiline /add    ##  file [file2 ...]");
            Console.WriteLine("  multiline /delete ##  file [file2 ...]");
            Console.WriteLine("    ##    line number to add/delete at");
            Console.WriteLine("    file  file(s) to change");
            Console.WriteLine();
            Console.WriteLine("  multiline /compare [/e] [##] file1 file2");
            Console.WriteLine("    /e    find lines that are equal, not unequal");
            Console.WriteLine("    ##    line number to start at, default is the first line");
            Console.WriteLine();
        }

        static bool CheckArgument(string arg, string full, int minlen)
        {
            return arg.Length > minlen && (arg[0] == '-' || arg[0] == '/') && full.StartsWith(arg.Substring(1).ToUpperInvariant());
        }

        static string[] GetFiles(string[] args, int start)
        {
            List<string> files = new List<string>(args.Length);
            for (int i = start; i < args.Length; ++i)
            {
                string file = args[i];
                if (file.IndexOf('*') >= 0 || file.IndexOf('?') >= 0)
                {
                    string dir = Path.GetDirectoryName(file);
                    dir = string.IsNullOrEmpty(dir) ? Directory.GetCurrentDirectory() : Path.GetFullPath(dir);
                    files.AddRange(Directory.GetFiles(dir, Path.GetFileName(file), SearchOption.TopDirectoryOnly));
                }
                else
                {
                    files.Add(Path.GetFullPath(file));
                }
            }
            return files.ToArray();
        }

        static string[] GetLines(string file)
        {
            try
            {
                return File.ReadAllLines(file);
            }
            catch (Exception ex) { Console.WriteLine("Failed to read '" + file + "': " + ex.Message); return null; }
        }

        static void WriteLines(string file, string[] lines)
        {
            try
            {
                StreamWriter w = new StreamWriter(file, false, Encoding.UTF8);
                for (uint i = 0; i < lines.Length; ++i)
                {
                    if (i == lines.Length - 1) w.Write(lines[i]);
                    else                       w.WriteLine(lines[i]);
                }
                w.Close();
            }
            catch (Exception ex) { Console.WriteLine("Failed to write '" + file + "': " + ex.Message); }
        }

        static int AddLine(uint insert, string[] files)
        {
            foreach (string file in files)
            {
                string[] lines = GetLines(file);
                if (lines == null) { continue; }

                uint len = (uint)lines.Length, after = len - insert;
                if (len < insert) { continue; }

                string[] new_lines = new string[len + 1];
                new_lines[insert] = "";
                Array.Copy(lines, 0, new_lines, 0, insert);
                if (after > 0)
                    Array.Copy(lines, insert, new_lines, insert + 1, after);

                WriteLines(file, new_lines);
            }

            return 0;
        }

        static int DeleteLine(uint delete, string[] files)
        {
            foreach (string file in files)
            {
                string[] lines = GetLines(file);
                if (lines == null) { continue; }

                uint len = (uint)lines.Length;
                if (len <= delete) { continue; }

                string[] new_lines = new string[--len];
                if (delete > 0)
                    Array.Copy(lines, 0, new_lines, 0, delete);
                Array.Copy(lines, delete + 1, new_lines, delete, len - delete);

                WriteLines(file, new_lines);
            }

            return 0;
        }

        static int CompareFiles(bool equal, uint offset, string file1, string file2)
        {
            string[] lines1 = GetLines(file1);
            if (lines1 == null) { return -11; }

            string[] lines2 = GetLines(file2);
            if (lines2 == null) { return -12; }

            uint max = (uint)Math.Min(lines1.Length, lines2.Length);
            for (uint i = offset; i < max; ++i)
            {
                if (equal && lines1[i] == lines2[i] || !equal && lines1[i] != lines2[i])
                {
                    Console.Write(equal ? "Equality" : "Difference");
                    Console.WriteLine(" found on line " + (i + 1));
                    Console.WriteLine("File 1: " + lines1[i]);
                    Console.WriteLine("File 2: " + lines2[i]);
                    return (int)(i + 1);
                }
            }

            if (lines1.Length == lines2.Length)
            {
                Console.WriteLine(equal ? "Every line is different, but the same number of lines" : "The files are the same");
                return 0;
            }
            else if (lines1.Length == max)
            {
                Console.WriteLine("'" + file2 + "' is longer, but up to the end of '" + file1 + "' the lines were " + (equal ? "different" : "identical"));
                return -1;
            }
            else if (lines2.Length == max)
            {
                Console.WriteLine("'" + file1 + "' is longer, but up to the end of '" + file2 + "' the lines were " + (equal ? "different" : "identical"));
                return -2;
            }

            return -13;
        }
    }
}
