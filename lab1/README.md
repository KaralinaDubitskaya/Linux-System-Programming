# The basic commands of the Unix OS

This script uses the **for** loop, outputting to the console the sizes and permissions for all files
in the specified directory and all its subdirectories (the directory name is specified by the user
as the first argument of the command line). The console displays the total number of files scanned.

1. The script must have the execute permission.                                              
                                                                                              
    To view the permissions, use:                                                 
<pre>
     > ls -l lab1.sh
</pre>
                                                                                            
    Allowing everyone to execute the script, enter:
<pre>
     > chmod +x lab1.sh
</pre>

2. To execute the script, enter:
<pre>
     > ./lab1.sh <i>directory_name</i>
</pre>
