#!/usr/bin/awk -f
BEGIN {
    filename = "_dummy";
    filenames["_dummy"] = "";
    linecov["_dummy"] = "";
    branchcov["_dummy"] = "";
    callcov["_dummy"] = "";
}
/^==>/ {
    filename = $2;
}
/source lines executed in function/ {
    filenames[$9] = filename;
    linecov[$9] = $1;
}
/calls executed in function/ {
    filenames[$8] = filename;
    callcov[$8] = $1;
}
/branches executed in function/ {
    filenames[$8] = filename;
    branchcov[$8] = $1;
}
END {
    OFS=","
#    print "function", "filenames", "linecov", "branchcov", "callcov";
    for (fn in filenames)
    {
    	if (fn == "_dummy") continue;
	if (match(fn, "_GLOBAL_")) continue;
	print fn, filenames[fn], linecov[fn], branchcov[fn], callcov[fn];
    }
}
