VERTFILE=./build/vert.spv
FRAGFILE=./build/frag.spv

cd ./glslShaders
if [ "$1" != "-f" ] && test -f "$VERTFILE"; then
    :
else
  glslc shader.vert -o build/vert.spv
  if [ $? -ne 0 ]; then
    echo "failed to compile vert shader"
  else
    echo "compiled vert shader"
  fi
fi
if [ "$1" != "-f" ] && test -f "$FRAGFILE"; then
    :
else
  glslc shader.frag -o build/frag.spv
  if [ $? -ne 0 ]; then
    echo "failed to compile frag shader"
  else
    echo "compiled fragshader"
  fi
fi
