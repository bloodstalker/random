#1/usr/bin/sh

PYTHON_ENV=$(python3 -c "import sys; sys.stdout.write('1') if hasattr(sys, 'real_prefix') else sys.stdout.write('0')")
if [[ $PYTHON_ENV == 1 ]]; then
  #"virtualenv" mistune && source ./mistune/bin/activate
  echo running inside a virtual env
else
  echo not running inside a virtual env
fi

#"git" clone https://github.com/lepture/mistune mistune-git && "python3" ./misutne-git/setup.py install

