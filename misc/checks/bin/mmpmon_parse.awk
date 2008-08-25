BEGIN { RS = " "; ORS=""; a=0 }

{
        if (a) {
                d[key] = $0;
                a = 0
        } else {
                key=$0; a=1
        }
}

END { if (length (d[val])==0) {print "0\n"} else {print d[val] "\n"} }
