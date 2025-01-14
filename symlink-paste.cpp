#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "Shlwapi.lib")

#include <windows.h>
#include <Ole2.h>
#include <ShellAPI.h>
#include <tchar.h>
#include <strsafe.h>
#include <Shlwapi.h>

enum PasteErrors { None = 0, InvalidClipboard, TooManyFiles, FileAlreadyExists, UnknownError };

#ifdef _UNICODE 
#ifdef _DEBUG
#define _msg(txt) ::MessageBox(NULL, (LPCWSTR)txt, (LPCWSTR)L"WC warning", MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2)
#define _dbg(txt) OutputDebugString(_T(txt))
#define _dbgW(txt) OutputDebugStringW(txt)
#else
#define _msg(txt) 
#define _dbg(txt) 
#define _dbgW(txt)
#endif
#else
#define _msg(txt) ::MessageBox(NULL, (LPCWSTR)_T(txt), (LPCWSTR)L"WC warning", MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2)
#define _dbg(txt) OutputDebugStringA(txt)
#endif

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	PasteErrors error = None;
	LPDATAOBJECT pData = 0;
	wchar_t sourceName[MAX_PATH];

	if (S_OK == OleGetClipboard(&pData))
	{
		FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		FORMATETC textformat = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM medium;

		if (S_OK == pData->GetData(&format, &medium))
		{
			HDROP hDrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));

			if (hDrop != NULL)
			{
				UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

				if (nFiles == 1)
				{
					DragQueryFile(hDrop, 0, sourceName, MAX_PATH);
				}
				else
				{
					error = TooManyFiles;
				}

				GlobalUnlock(medium.hGlobal);
			}
			else
			{
				error = InvalidClipboard;
			}

			ReleaseStgMedium(&medium);
		}
		else if (S_OK == pData->GetData(&textformat, &medium))
		{
			LPCTSTR ptxt = (LPCTSTR)GlobalLock(medium.hGlobal);

			if (ptxt)
			{
				StringCchCopy(sourceName, MAX_PATH, ptxt);
				GlobalUnlock(medium.hGlobal);
			}
			else
			{
				error = InvalidClipboard;
			}

			ReleaseStgMedium(&medium);
		}
		else
		{
			error = InvalidClipboard;
		}

		pData->Release();
	}
	else
	{
		error = InvalidClipboard;
	}

	if (error == None)
	{
		sourceName[MAX_PATH - 1] = 0;

		if (!PathIsRelative(sourceName) && PathFileExists(sourceName))
		{
			wchar_t targetName[MAX_PATH];
			wchar_t targetPath[MAX_PATH];

			StringCchCopy(targetPath, MAX_PATH, lpCmdLine);
			StringCchCopy(targetName, MAX_PATH, sourceName);
			PathStripPath(targetName);

			//StringCchCat(targetPath, MAX_PATH, targetName);



			_msg(__wargv[1]);

#if 0
			if (!SetCurrentDirectory(targetPath))
			{
				wchar_t error[MAX_PATH];
				wsprintf(error, _T("SetCurrentDirectory failed (%d)\n"), GetLastError());
				_msg(error);

				return 0;
			}
			if (!PathFileExists(targetName))
#else
			StringCchCopy(targetPath, MAX_PATH, __wargv[1]);
			StringCchCat(targetPath, MAX_PATH, L"\\");
			StringCchCat(targetPath, MAX_PATH, targetName);

			if (!PathFileExists(targetPath))
#endif
			{
				if (!CreateSymbolicLink(targetPath, sourceName, PathIsDirectory(sourceName) ? SYMBOLIC_LINK_FLAG_DIRECTORY : NULL))
				{
					error = UnknownError;
				}
			}
			else
			{
				error = FileAlreadyExists;
			}
		}
		else
		{
			error = InvalidClipboard;
		}
	}

	if (error == InvalidClipboard)
	{
		MessageBox(NULL, L"You have to copy a file or folder first.",  L"Paste Symlink", MB_ICONERROR);
	}
	else if (error == TooManyFiles)
	{
		MessageBox(NULL, L"Only one file or folder at a time, please.",  L"Paste Symlink", MB_ICONERROR);
	}
	else if (error == FileAlreadyExists)
	{
		MessageBox(NULL, L"A file with the same name already exists.",  L"Paste Symlink", MB_ICONERROR);
	}
	else if (error == UnknownError)
	{
		MessageBox(NULL, L"Couldn't create the symbolic link.",  L"Paste Symlink", MB_ICONERROR);
	}

	return 0;
}
