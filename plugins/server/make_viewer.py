import json
import re

r_host = re.compile('var HOST = .*;')
r_target = re.compile('var TARGETS = \[.*\];')
with open('view.html', 'r') as view:
	with open('viewer.h', 'w') as out:
		print( "#pragma once", file=out)
		print( "#include <string>", file=out)
		print( 'const std::string viewer(std::string host, const std::vector<std::string> & targets) {', file=out)
		print( '    std::string target_concat = "";', file=out)
		print( '    for(auto t: targets) {', file=out)
		print( '        if (target_concat.size()) target_concat += ",";', file=out)
		print( '        target_concat += "\\""+t+"\\"";', file=out)
		print( '    }', file=out)
		print( '    std::string res = ', file=out)
		
		for l in view:
			jl = json.dumps(l)
			jl = r_host.sub('var HOST = \\""+host+"\\";', jl)
			jl = r_target.sub('var TARGETS = ["+target_concat+"];', jl)
			print( jl, file=out )
		print( "; return res;}", file=out )


