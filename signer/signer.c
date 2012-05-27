/*
    signer: tool for self-signing
    Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
*/

// The main program document
// Parses arguments and calls the appropriate certificate functions

#include "stdafx.h"
#include "signer.h"
#include "errors.h"

#define COMMAND_LIST	0
#define COMMAND_CREATE	1
#define COMMAND_SIGN	2
#define COMMAND_EXPORT	3
#define COMMAND_INSTALL	4

static const WCHAR* commands[] = { L"list", L"create", L"sign", L"export", L"install" };
static const int min_args[] = { 0, 1, 2, 2, 2 };
static const int max_args[] = { 0, 1, -1, 3, 2 };
static inline BOOL is_command(const WCHAR *arg) { return arg[0] == '/' || arg[0]  == '-'; }
static inline BOOL only_letter(const WCHAR *x, WCHAR l) { return (x[0] == l || towlower(x[0]) == l) && x[1] == 0; }

static void About() {
	fwprintf(stderr, L"signer Copyright (C) 2011  Jeffrey Bush <jeff@coderforlife.com>\n");
	fwprintf(stderr, L"This program comes with ABSOLUTELY NO WARRANTY;\n");
	fwprintf(stderr, L"This is free software, and you are welcome to redistribute it\n");
	fwprintf(stderr, L"under certain conditions;\n");
	fwprintf(stderr, L"See http://www.gnu.org/licenses/gpl.html for more details.\n\n");
}

static WCHAR *program = NULL;
static int Usage() {
	size_t len = wcslen(program = PathFindFileName(program));
	if (len > 4 && program[len-4] == L'.' && program[len-3] == L'e' && program[len-2] == L'x' && program[len-1] == L'e')
		program[len-4] = 0;

	fwprintf(stderr, L"Usage: %s [/nologo] /command [/machine] args\n\n", program);
	fwprintf(stderr, L"  /nologo   don't show the about information\n");
	fwprintf(stderr, L"  /machine  use machine cert store instead of current user\n\n");
	fwprintf(stderr, L"The /command can be one of the following (or just first letter, eg /l):\n");
	fwprintf(stderr, L"  /list     lists certificates that can be used to sign\n            No Arguments.\n");
	fwprintf(stderr, L"  /create   creates a certificate to be used for signing\n            args: cert_name\n");
	fwprintf(stderr, L"  /sign     signs files using a certificate, creates the cert if necessary\n            args: cert_name file ...\n");
	fwprintf(stderr, L"  /export   exports a certificate to a file\n            args: [/CA] cert_name file\n");
	fwprintf(stderr, L"  /install  install a certificate into an off-line registry hive\n            args: cert_name reg_hive_file\n");
	fwprintf(stderr, L"\n");
	fwprintf(stderr, L"Runs with normal privileges except the following require admin command line:\n  /machine, /install\n");
	fwprintf(stderr, L"\n");
	return -1;
}

static int list(BOOL machine, int argc, WCHAR *argv[]) {
	DWORD err;
	HCERTSTORE hCertStore;
	PCCERT_CONTEXT cert = NULL;
	WCHAR name[512];
	BOOL any = FALSE;
	if ((hCertStore = OpenStore(TrustedPublisher, machine)) == NULL) return last_error(L"Failed to open store");
	while ((cert = CertEnumCertificatesInStore(hCertStore, cert)) != NULL) {
		if (HasPrivateKey(cert)) { // has private key available
			if (CertGetNameString(cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, name, ARRAYSIZE(name))) {
				any = TRUE;
				wprintf(L"%s\n", name);
			} else {
				/* error! */
				last_error(L"Failed to get certificate name");
			}
		}
	}
	err = GetLastError();
	if (err != CRYPT_E_NOT_FOUND && err != ERROR_NO_MORE_FILES) { /* error! */ last_error(L"Failed to fully list certificates"); }
	Cleanup(hCertStore, NULL);
	if (!any) wprintf(L"[No certificates available]\n");
	return 0;
}

static int create(BOOL machine, int argc, WCHAR *argv[]) {
	HCERTSTORE hCertStore;
	PCCERT_CONTEXT cert;
	if ((hCertStore = OpenStore(TrustedPublisher, machine)) == NULL) return last_error(L"Failed to open store");
	if ((cert = CreateCert(argv[0], machine, hCertStore)) == NULL) return last_error_and_close_store(L"Failed to create certificate", hCertStore);
	Cleanup(hCertStore, cert);
	wprintf(L"Created code-signing certificate %s\n", argv[0]);
	return 0;
}

static int sign(BOOL machine, int argc, WCHAR *argv[]) {
	HCERTSTORE hCertStore;
	PCCERT_CONTEXT cert;
	int i;

	if (!PrepareToSign()) return last_error(L"Failed to load the signing functions");
	if ((hCertStore = OpenStore(TrustedPublisher, machine)) == NULL) return last_error(L"Failed to open store");
	if ((cert = FindOrCreateCert(argv[0], machine, TRUE, hCertStore)) == NULL) return last_error_and_close_store(L"Failed to find or create certificate", hCertStore);

	// Go through all files
	for (i = 1; i < argc; ++i) {
		if (!PathFileExists(argv[i]))
			last_error_str(L"File '%s' does not exist", argv[i]);
		else if (!Sign(argv[i], cert, hCertStore))
			last_error_str(L"Failed to sign '%s'", argv[i]);
		else
			wprintf(L"Signed '%s'\n", argv[i]);
	}

	Cleanup(hCertStore, cert);
	return 0;
}

static int exprt(BOOL machine, int argc, WCHAR *argv[]) {
	int err;
	HCERTSTORE hCertStore = 0;
	PCCERT_CONTEXT cert = NULL;
	BOOL ca = is_command(*argv) && _wcsicmp(*argv+1, L"ca") == 0;

	if (ca) { --argc; ++argv; }
	if (argc != 2) return Usage();

	if ((err = FindCertInStore(argv[0], ca, machine, &cert, &hCertStore)) != 0) return err;

	err = Export(cert, argv[1]);

	Cleanup(hCertStore, cert);
	if (!err) wprintf(L"Exported '%s%s' to '%s'\n", argv[0], (ca ? L" Certificate Authority" : L""), argv[1]);
	return err;
}

static int install_part(BOOL machine, BOOL ca, const WCHAR *name, const WCHAR *file) {
	int err;
	HCERTSTORE hCertStore = 0;
	PCCERT_CONTEXT cert = NULL;
	if ((err = FindCertInStore(name, ca, machine, &cert, &hCertStore)) != 0) return err;

	err = Install(file, ca ? Root : TrustedPublisher, cert);

	Cleanup(hCertStore, cert);
	if (!err) wprintf(L"Installed '%s%s' into '%s'\n", name, (ca ? L" Certificate Authority" : L""), file);
	return err;
}

static int install(BOOL machine, int argc, WCHAR *argv[]) {
	int err;
	if (!PrepareToInstall()) return last_error(L"Failed to enable take ownership, backup, and restore privileges");
	if ((err = install_part(machine, TRUE, argv[0], argv[1])) == 0)
		err = install_part(machine, FALSE, argv[0], argv[1]);
	return err;
}

int wmain(int argc, WCHAR *argv[])
{
	int i;
	const WCHAR *cmd;
	BOOL machine = FALSE;

	program = *argv;
	--argc; ++argv;

	// Check for nologo argument
	if (argc > 0 && is_command(*argv) && _wcsicmp(*argv+1, L"nologo") == 0) {
		--argc; ++argv;
	} else {
		About();
	}

	// Check for enough arguments
	if (argc == 0 || !is_command(*argv))
		return Usage();

	// Look for the command to execute
	cmd = *argv+1;
	for (i = 0; i < ARRAYSIZE(commands); ++i)
		if (only_letter(cmd, commands[i][0]) || _wcsicmp(cmd, commands[i]) == 0)
			break;
	if (i == ARRAYSIZE(commands)) // no command found
		return Usage();
	--argc; ++argv;

	// Check for machine argument
	if (argc > 0 && is_command(*argv)) {
		machine = only_letter(*argv+1, L'm') || _wcsicmp(*argv+1, L"machine") == 0;
		if (machine) { --argc; ++argv; }
	}

	// Make sure there are the appropriate number of arguments for the command
	if (argc < min_args[i] || (max_args[i] >= 0 && argc > max_args[i]))
		return Usage();

	// Call the command
	switch (i) {
	case COMMAND_LIST:		return list(machine, argc, argv);
	case COMMAND_CREATE:	return create(machine, argc, argv);
	case COMMAND_SIGN:		return sign(machine, argc, argv);
	case COMMAND_EXPORT:	return exprt(machine, argc, argv);
	case COMMAND_INSTALL:	return install(machine, argc, argv);
	}
	
	return -2;
}
