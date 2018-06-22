from __future__ import print_function
functions = """
HRESULT __stdcall DirectInput8Create(
         HINSTANCE hinst,
         DWORD dwVersion,
         REFIID riidltf,
         LPVOID * ppvOut,
         LPUNKNOWN punkOuter
);
HRESULT __stdcall DllCanUnloadNow(void);
HRESULT __stdcall DllGetClassObject(
  _In_  REFCLSID rclsid,
  _In_  REFIID   riid,
  _Out_ LPVOID   *ppv
);
HRESULT __stdcall DllRegisterServer(void);
HRESULT __stdcall DllUnregisterServer(void);
"""

import re
fr = re.compile("(\S+) +?(\S* )? *?(\S+)\((.*?)\);", re.MULTILINE | re.DOTALL)
pr = re.compile("([\w\s]+?[\*\& ])\s*(\w+)")
funcs = []
for m in fr.findall(functions.replace('__stdcall', 'WINAPI').replace('_In_','').replace('_Out_','')):
	f = {}
	f['ret'], f['dec'], f['name'] = [a.strip() for a in m[:3]]
	f['args'] = [pr.match(a.strip()).groups() if a.strip()!='void' else ('void',None) for a in m[3].split(',')]
	funcs.append(f)

dll_name = 'dinput8'
for f in funcs:
	print( 'typedef %s(%s *%sT)(%s);'%(f['ret'], f['dec'], f['name'], ', '.join([a[0] for a in f['args']])) )
	print( '%sT %s;'%(f['name'],f['name']) )
print()

for f in funcs:
	print( '%s = (%sT)GetProcAddress(dll, "%s");'%(f['name'], f['name'], f['name']) )
print()

for f in funcs:
	print( '%s = 0;'%(f['name']) )
print()

for f in funcs:
	print( '%s %s %s(%s) {'%(f['ret'], f['dec'], f['name'], ', '.join([a[0] if a[1] is None else a[0]+' '+a[1] for a in f['args']])) )
	print( '    return %s.%s(%s);'%(dll_name, f['name'], ', '.join([a[1] for a in f['args'] if a[1] is not None])) )
	print('}')
print()