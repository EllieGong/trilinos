<? 

# --------------------------------------------------- #
# File Locations                                      #
#                                                     #
# Specify here the location of the matrix to be       #
# uploaded and the script files.                      #
#                                                     #
# `MatrixDirectory' specifies where to store the      #
# uploaded matrices.                                  #
#                                                     #
# `ImageDirectory' specifies where to store the       #
# images created on-the-fly.                          #
#                                                     #
# `TempDirectory' is the directory used to store any  #
# temporary data.                                     #
#                                                     #
# `PythonDirectory' is the directory containing the   #
# step_process.py; typically, this is the ml/python/  #
# MatrixPortal directory.                             #
# --------------------------------------------------- #

$MACHINE = "givens4";

if ($MACHINE == "givens4")
{
  $MatrixDirectory = "/home/chinella/Web/MatrixPortal/HBMatrices/";
  $ImageDirectory = "/home/chinella/Web/MatrixPortal/tmp/";
  $TempDirectory = "/tmp/";
  $PythonDirectory = "/home/chinella/Web/MatrixPortal/";
  $HTMLImageDirectory = "http://givens4.ethz.ch/MatrixPortal/tmp";
  $PYTHONPATH = "/home/masala/Trilinos/LINUX_MPI/lib/python2.3/site-packages/:/home/masala/lib/python2.3/site-packages/";
  $LD_LIBRARY_PATH = "/home/masala/Trilinos/LINUX_MPI/lib";
  $ENABLE_MPI = TRUE;
  $MAX_PROCS = 25;
  $MPI_BOOT = "lamboot";
  $MPI_HALT = "lamhalt";
}
else if ($MACHINE == "kythira")
{
  $MatrixDirectory = "/var/www/html/MatrixPortal/HBMatrices/";
  $ImageDirectory = "/var/www/html/MatrixPortal/tmp/";
  $TempDirectory = "/tmp/";
  $PythonDirectory = "/home/msala/Trilinos/packages/ml/python/MatrixPortal";
  $HTMLImageDirectory = "http://kythira.ethz.ch/MatrixPortal/tmp";
  $PYTHONPATH = "/home/masala/Trilinos/LINUX_MPI/lib/python2.3/site-packages/:/home/masala/lib/python2.3/site-packages/";
  $LD_LIBRARY_PATH = "/home/masala/Trilinos/LINUX_MPI/lib";
  $ENABLE_MPI = FALSE;
  $MAX_PROCS = 1;
  $MPI_BOOT = "";
  $MPI_HALT = "";
}

# --------------------------------------------------- #
# Web Page Styles and Appearance                      #
#                                                     #
# Functions top_left() and top_right() can be used to #
# personalize the appearance of the web page.         #
# --------------------------------------------------- #

?>

<?  function top_left() { ?>
          <a href=index.html"
            <a href="index.html"
            <img src=common/matrix_portal_logo.png height=60 border=0 alt="The Matrix Portal" /></a>
<?  } ?>

<? function top_right() { ?>
        <a href="http://www.ethz.ch/"
            <img src=common/eth_logo.gif border=0 alt="ETHZ" /></a>
<?  } ?>
