# A simple code generator using the SDK functions in the base controller

import re

SDK = open('../../SDK/sdk.h','r').read()
m = re.search("""class BaseGameController.*?{(.*?)};""", SDK, re.DOTALL | re.MULTILINE)
code = m.group(1)

fn_r = re.compile(' (\w*)\(')

callbacks = []
states = []
functions = []
for f in re.findall("""virtual (.*?)[;{]""", code):
	fn = f.replace('= 0','').strip()
	if ' on' in fn:
		callbacks.append( fn )
	elif ' provide' in fn:
		states.append( fn )
	elif '~' not in fn:
		functions.append( fn )

print()
print()
print( '-'*10, 'function definition', '-'*10)
for f in callbacks:
	if 'bool ' in f:
		print( '\t%s {return false;}'%f )
	else:
		print( '\t%s {}'%f )

print()

map_name_cnt = {}
for f in functions:
	fn = fn_r.search(f).group(1)
	if fn in map_name_cnt:
		map_name_cnt[fn] += 1
		f = f.replace(fn, fn+'_%s'%map_name_cnt[fn])
	else:
		map_name_cnt[fn] = 0
	print( '\t%s;'%f )

def underscore(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def parse_args(f):
	all_args = re.search('\((.*)\)', f).group(1).strip()
	if all_args:
		return [re.sub('=.*', '', a).split()[-1] for a in all_args.split(',')]
	return []

print()
print()
print( '-'*10, 'python caller', '-'*10)
fn_r = re.compile(' (\w*)\(')
for f in callbacks:
	args = parse_args(f)
	ret = 'return '
	if 'void ' in f: ret = ''
	print( '\tvirtual %s final {'%f )
	print( '\t\t%scallAll("%s"%s);'%(ret, underscore(fn_r.search(f).group(1)), ', '.join(['']+args)) )
	print( '\t}' )
	
print()
print()
print( '-'*10, 'base decl', '-'*10)
map_name_cnt = {}
for f in functions:
	fn = fn_r.search(f).group(1)
	if fn in map_name_cnt:
		map_name_cnt[fn] += 1
		f = f.replace(fn, fn+'_%s'%map_name_cnt[fn])
	else:
		map_name_cnt[fn] = 0
	args = parse_args(f)
	ret = 'return '
	if 'void ' in f: ret = ''
	print( '%s { %smain_->%s(%s); }'%(re.sub(' *= .*?([,)])', '\\1', f.replace(fn, 'BasePythonController::'+fn)), ret, fn, ', '.join(args)) )
	
print()
print()
print( '-'*10, 'cmd decl', '-'*10)
map_name_cnt = {}
for f in functions:
	args = parse_args(f)
	fn = fn_r.search(f).group(1)
	p_fn = underscore(fn)
	if fn in map_name_cnt:
		map_name_cnt[fn] += 1
		fn = fn+'_%s'%map_name_cnt[fn]
	else:
		map_name_cnt[fn] = 0

	df = '.def'
	if len(args) == 0:
		df = '.def_property_readonly'
	print( '\t\t%s("%s", &BasePythonController::%s, py::call_guard<py::gil_scoped_release>())'%(df, p_fn, fn))
