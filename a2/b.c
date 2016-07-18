static int                  /* we return whatever func() returns */
dopath(Myfunc* func, char* path)
{
    struct stat     statbuf;
    struct dirent   *dirp;
    DIR             *dp;
    int             ret;

    if (lstat(path, &statbuf) < 0) /* stat error */
        return(func(path, &statbuf, FTW_NS));
    if (S_ISDIR(statbuf.st_mode) == 0) /* not a directory */
        return(func(path, &statbuf, FTW_F));

 /*
 * It's a directory. First call func() for the directory,
 * then process each filename in the directory.
 */
    if ((ret = func(path, &statbuf, FTW_D)) != 0)
        return(ret);

    if ( chdir(path) < 0 )
      return(func(path, &statbuf, FTW_DNR));

     if ((dp = opendir(".")) == NULL)     /* can't read directory */
         return(func(path, &statbuf, FTW_DNR));

     while ((dirp = readdir(dp)) != NULL) {
         if (strcmp(dirp->d_name, ".") == 0 ||
             strcmp(dirp->d_name, "..") == 0)
                 continue;        /* ignore dot and dot-dot */

         if ((ret = dopath(func, dirp->d_name)) != 0)          /* recursive */
              break; /* time to leave */
     }
     if ( chdir("..") < 0 )
       err_ret("can't go up directory");

     if (closedir(dp) < 0)
         err_ret("can't close directory %s", fullpath);

     return(ret);
}
