// xml-compact: XML compactor (white-space remover)
// Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>
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
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace xml_compact
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("xml-compact Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>");
            Console.WriteLine("This program comes with ABSOLUTELY NO WARRANTY;");
            Console.WriteLine("This is free software, and you are welcome to redistribute it");
            Console.WriteLine("under certain conditions;");
            Console.WriteLine("See http://www.gnu.org/licenses/gpl.html for more details.");
            Console.WriteLine();

            if (args.Length != 2) {
                Console.WriteLine("Must provide input and output XML files on the command line");
                return;
            }

            try {
                string txt = File.ReadAllText(args[0], Encoding.UTF8);

                txt = Regex.Replace(txt, @"<!\s*--([^-]|-[^-])*--\s*>", "");     // comments
                txt = Regex.Replace(txt, @"(['""\w])\s+(/?>)",          "$1$2"); // extra spaces at the ends of tags
                txt = Regex.Replace(txt, @">\s+",                       ">");    // extra spaces after >
                txt = Regex.Replace(txt, @"\s+<",                       "<");    // extra spaces before <
                txt = Regex.Replace(txt, @"\s+",                        " ");    // all multi-spaces

                File.WriteAllText(args[1], txt, Encoding.UTF8);
            } catch (Exception e) {
                Console.Error.WriteLine("Error: "+e.Message);
            }

            return;
        }
    }
}
