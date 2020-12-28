#CS 5348 Project <br /> 
You need to implement the following commands: Keep in mind that all files are small files. No large files at all for this project.<br /> 
(a) rm v6-file<br /> 
Delete the file v6_file from the v6 file system.<br /> 
Remove all the data blocks of the file, free the i-node and remove the directory entry.<br /> 
(b) mkdir v6dir<br /> 
create the v6dir. It should have two entries . and ..<br /> 
(c) cd v6dir<br /> 
change working directory of the v6 file system to the v6dir<br /> 
(d) q<br /> 
Save all changes and quit. Keep in mind that all file names starting with / are absolute path names and those not starting with / are relative to current working directory.
Keep in mind that cpin and cpout must be changed so that they allow file names to be long path names (such as /user/Venky/myfile) with directories allowed in the file name.
For example the following sequence,<br /> 
mkdir /user<br /> 
mkdir /user/Venky<br /> 
cd /user/Venky<br /> 
cpin myfile-in-unix f1<br /> 
is possible where f1 is created in the directory /user/Venky of the v6 file system
