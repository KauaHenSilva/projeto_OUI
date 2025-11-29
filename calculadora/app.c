#include <stdio.h>
#include "dep/c2wasm.c"
#include "dep/react.c"

ReactRoot root;
void rootRender();

void handleClick() {
  c2wasm_js_var args = c2wasm_create_array();
  c2wasm_append_array_string(args, "Hello from C-React! You clicked a button made with C code.");
  c2wasm_call_object_prop(c2wasm_window, "alert", args);
}

void rootRender() {
  ReactComponent main_component = ReactCreateElement(
    "div",
    ReactCreateProps(
      "className", ReactCreateString("container"),
      "style", ReactCreateProps(
        "padding", ReactCreateString("20px"),
        "maxWidth", ReactCreateString("800px"),
        "margin", ReactCreateString("0 auto")
      )
    ),

    ReactCreateElement("h1",
      ReactCreateProps(
        "style", ReactCreateProps(
          "color", ReactCreateString("#333"),
          "borderBottom", ReactCreateString("2px solid #eee"),
          "paddingBottom", ReactCreateString("10px")
        )
      ),
      ReactCreateString("Welcome to C-React")
    ),

    ReactCreateFragment(
      ReactCreateElement("p", ReactNULL,
        ReactCreateString("This webserver is built with C code compiled to WebAssembly.")
      ),

      ReactCreateElement("p", ReactNULL,
        ReactCreateString("Your C code runs efficiently in the browser.")
      ),

      ReactCreateElement(
        "button",
        ReactCreateProps(
          "onClick", ReactCreateClickHandler(handleClick),
          "style", ReactCreateProps(
            "padding", ReactCreateString("12px 20px"),
            "backgroundColor", ReactCreateString("#0d6efd"),
            "color", ReactCreateString("white"),
            "border", ReactCreateString("none"),
            "borderRadius", ReactCreateString("5px"),
            "cursor", ReactCreateString("pointer"),
            "fontSize", ReactCreateString("16px"),
            "marginTop", ReactCreateString("20px")
          )
        ),
        ReactCreateString("Click Me")
      )
    )
  );
  ReactRootRender(root, main_component);
}

int main() {
  ReactStart();
  root = ReactDOMCreateRoot(ReactGetElementById("root"));
  rootRender();
  return 0;
}