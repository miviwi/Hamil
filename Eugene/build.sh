#! /bin/bash
cwd="$(pwd)"
dp0="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$dp0/sysv"

if [ ! -d build ]; then
  mkdir build
fi

python setup.py build
for filename in $(find -type f -name "*.so"); do
  cp "$filename" ../src
done

cd "$cwd"

printf "\n        ...Done!\n"
