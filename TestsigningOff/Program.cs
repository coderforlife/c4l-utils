// TestsigningOff: Turn off testsigning or nointegritychecks
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

#define DO_TESTSIGNING // when commented out does NOINTEGRITYCHECKS

using System;
using System.Collections.Generic;
using System.Text;
using System.Management;

using WMI;
using BcdConstants;

namespace TestsigningOff
{
    class Program
    {
        
#if DO_TESTSIGNING
        const BcdLibraryElementTypes ID = BcdLibraryElementTypes.Boolean_AllowPrereleaseSignatures;
        const string NAME = "TESTSIGNING";
#else
        const BcdLibraryElementTypes ID = BcdLibraryElementTypes.Boolean_DisableIntegrityChecks;
        const string NAME = "NOINTEGRITYCHECKS";
#endif

        static int Main(string[] args)
        {
            Console.WriteLine("TestsigningOff Copyright (C) 2010-2012  Jeffrey Bush <jeff@coderforlife.com>");
            Console.WriteLine("This program comes with ABSOLUTELY NO WARRANTY;");
            Console.WriteLine("This is free software, and you are welcome to redistribute it");
            Console.WriteLine("under certain conditions;");
            Console.WriteLine("See http://www.gnu.org/licenses/gpl.html for more details.");
            Console.WriteLine();

            ConnectionOptions opts = new ConnectionOptions();
            opts.Impersonation = ImpersonationLevel.Impersonate;
            opts.EnablePrivileges = true;

            ManagementScope scope = new ManagementScope("root\\WMI", opts);

            // Open the system store
            BcdObject store = new BcdObject(scope, Guids.SystemStore, "");

            // Get the Bootmgr information
            string bm_name = GetString(store, BcdLibraryElementTypes.String_Description);
            bool bm_ts = Has(store, ID);

            // Get the default GUID
            string def = null;
            try
            {
                def = GetString(store, BcdBootMgrElementTypes.Object_DefaultObject);
            }
            catch (Exception) {}

            // Get the current GUID
            BcdObject current = new BcdObject(scope, Guids.Current, "");
            string cur = current.Id;

            // Loop over all OS GUIDs
            string[] guids = GetIds(store, BcdBootMgrElementTypes.ObjectList_DisplayOrder);
            BcdObject[] objs = new BcdObject[guids.Length];
            string[] names = new string[guids.Length];
            bool[] testsignings = new bool[guids.Length];
            bool any_ts = bm_ts;
            int def_i = -1, cur_i = -1;
            for (int i = 0; i < guids.Length; i++)
            {
                objs[i] = new BcdObject(scope, guids[i], "");
                names[i] = GetString(objs[i], BcdLibraryElementTypes.String_Description);
                testsignings[i] = Has(objs[i], ID);
                any_ts |= testsignings[i];
                if (guids[i].Equals(def)) def_i = i;
                if (guids[i].Equals(cur)) cur_i = i;
            }

            // Final message
            if (any_ts)
            {
                string msg = "You have " + NAME + " entries for the following:\n";
                if (bm_ts)
                    msg += "  {bootmgr} "+bm_name+"\n";
                if (cur_i >= 0 && testsignings[cur_i])
                    msg += "  {current} "+names[cur_i]+"\n";
                if (def_i >= 0 && cur_i != def_i && testsignings[def_i])
                    msg += "  {default} "+names[def_i]+"\n";
                for (int i = 0; i < guids.Length; i++)
                {
                    if (i != cur_i && i != def_i && testsignings[i])
                        msg += "  "+guids[i]+" "+names[i]+"\n";
                }
                msg += "If these were added with a previous version of this program you should remove them. Would you like to remove " + NAME + "? [Y/n] ";
                Console.WriteLine(msg);

                // y/n input
                string line;
                while ((line = Console.ReadLine()) != null)
                {
                    line = line.Trim();
                    if (line.Length == 0 || line[0] == 'Y' || line[0] == 'y')
                        break;
                    else if (line[0] == 'n' || line[0] == 'N')
                    {
                        Console.WriteLine("Nothing was done.");
                        return 0;
                    }
                }

                // Set the data
                if (bm_ts)	Remove(store, ID);
                for (int i = 0; i < guids.Length; i++)
                    if (testsignings[i])
                        Remove(objs[i], ID);
                Console.WriteLine("Removed " + NAME + ".");
            }
            else
            {
                Console.WriteLine("Nothing has " + NAME + ".");
            }

            return 0;
        }

        static string[] GetIds<E>(BcdObject o, E t) where E : IConvertible { return GetIds(o, Convert.ToUInt32(t)); }
        static string[] GetIds(BcdObject o, uint t)
        {
            ManagementBaseObject mbo;
            return o.GetElement(t, out mbo) ? (string[])mbo.GetPropertyValue("Ids") : null;
        }

        static string GetString<E>(BcdObject o, E t) where E : IConvertible { return GetString(o, Convert.ToUInt32(t)); }
        static string GetString(BcdObject o, uint t)
        {
            ManagementBaseObject mbo;
            return o.GetElement(t, out mbo) ? (string)mbo.GetPropertyValue("String") : null;
        }

        static bool Has<E>(BcdObject o, E t) where E : IConvertible { return Has(o, Convert.ToUInt32(t)); }
        static bool Has(BcdObject o, uint t)
        {
            ManagementBaseObject mbo;
            return o.GetElement(t, out mbo);
        }

        static void Remove<E>(BcdObject o, E t) where E : IConvertible { Remove(o, Convert.ToUInt32(t)); }
        static void Remove(BcdObject o, uint t)
        {
            if (!o.DeleteElement(t))
                throw new InvalidOperationException();
        }
    }
}
