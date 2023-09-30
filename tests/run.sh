
if ["$1" == ""]; then
  for fn in $(ls tests)
  do
    if [ "$fn" = "run.sh" ]; then
      continue
    fi
    rm test-index*
    rm a.out
    echo $fn
    clang++ tests/${fn} -I/opt/homebrew/Cellar/googletest/1.14.0/include -std=c++20 -L/opt/homebrew/Cellar/googletest/1.14.0/lib -lgtest && ./a.out
  done
else
  rm test-index*
  rm a.out
  clang++ $1 -I/opt/homebrew/Cellar/googletest/1.14.0/include -std=c++20 -L/opt/homebrew/Cellar/googletest/1.14.0/lib -lgtest && ./a.out
fi

