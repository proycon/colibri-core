# $Id: bootstrap 8036 2011-01-27 11:56:11Z joostvb $
# $URL: https://ilk.uvt.nl/svn/sources/ucto/trunk/bootstrap $

# bootstrap - script to bootstrap the distribution rolling engine

# usage:
#  $ sh ./bootstrap && ./configure && make dist[check]
#
# this yields a tarball which one can install doing
#
#  $ tar zxf PACKAGENAME-*.tar.gz
#  $ cd PACKAGENAME-*
#  $ ./configure
#  $ make
#  # make install

# requirements:
#  GNU autoconf, from e.g. ftp.gnu.org:/pub/gnu/autoconf/
#  GNU automake, from e.g. http://ftp.gnu.org/gnu/automake/


automake=automake
aclocal=aclocal

if [ ! -f README ]; then
    ln -s README.md README
fi

# if you want to autogenerate a ChangeLog form svn:
# 
#  svn2cl, a python script, as used in the GNU Enterprise project.
#    By jcater (Jason Cater), contributions by reinhard (Reinhard Müller).
#    Get it from
#    http://www.gnuenterprise.org/cgi-bin/viewcvs.cgi/*checkout*/gnue/trunk/gnue-common/utils/svn2cl .
#    svn2cl is used in Makefile.am too.
#
# (Another svn2cl implementation, in perl, is at
# http://www.contactor.se/~dast/svn/archive-2002-04/0910.shtml)
#
# see also toplevel Makefile.am

# test -f ChangeLog || {
#   svn log --verbose > ChangeLog
#}

# inspired by hack as used in mcl (from http://micans.org/)

# autoconf-archive Debian package, aclocal-archive RPM, obsolete/badly supported OS, installed in home dir
acdirs="/usr/share/autoconf-archive/ /usr/share/aclocal/ /usr/local/share/autoconf-archive/ /usr/local/share/aclocal/ $HOME/local/share/autoconf-archive/"

found=false
for d in $acdirs
do
    if test -f ${d}libtool.m4
    then
        echo "Autoconf archive found in $d, good"
        found=true
        break
    fi
done

if ! $found
then
    cat <<EOT
WARNING!!! The Autoconf Archive was not found. If anything goes wrong, this is
most likely the cause! Look for the autoconf-archive package in your
distribution. If you use Debian, Ubuntu, or Homebrew on Mac OS X, then such a
package will be available, other distributions may use alternate names such as
aclocal-archive or ac-archive. Alternatively, you could install the GNU Autoconf Macro
Archive's http://autoconf-archive.cryp.to/ac_path_lib.html
as `pwd`/acinclude.m4.
EOT
fi


if $automake --version|head -1 |grep ' 1\.[4-8]'; then
    echo "automake 1.4-1.8 is active. You should use automake 1.9 or later"
    if test -f /etc/debian_version; then
        echo " sudo apt-get install automake1.9"
        echo " sudo update-alternatives --config automake"
    fi
    exit 1
fi

if $aclocal --version|head -1 |grep ' 1\.[4-8]'; then
    echo "aclocal 1.4-1.8 is active. You should use aclocal 1.9 or later"
    if test -f /etc/debian_version; then	
        echo " sudo apt-get install aclocal1.9"
        echo " sudo update-alternatives --config aclocal"
    fi
    exit 1
fi

# Debian automake package installs as automake-version.  Use this
# to make sure the right automake is being used.
# if not installed, use: apt-get install automake1.9

AUTOMAKE=automake ACLOCAL=aclocal autoreconf --install --symlink

# add --make if you want to run "make" too.

# autoreconf should run something like:
#
# aclocal-1.9 \
#     && automake-1.9 --add-missing --verbose --gnu \
#     && autoconf

