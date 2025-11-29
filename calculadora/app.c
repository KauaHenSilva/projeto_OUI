#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dep/c2wasm.c"
#include "dep/react.c"

// --- Definições Globais ---
ReactRoot root;
void rootRender();

// --- Estado da Calculadora ---
char displayBuffer[64] = "0";     // Número principal
char historyBuffer[128] = "";     // Histórico (Aumentado para caber a conta toda)
double storedValue = 0;           // Primeiro valor da operação
char currentOp = 0;               // Operador atual
int waitingForOperand = 0;        // Flag: esperando o segundo número?
int resultJustShown = 0;          // Flag: acabamos de mostrar um resultado?

// --- Auxiliares ---

double getDisplayValue() {
    return atof(displayBuffer);
}

void formatDisplay(double value) {
    sprintf(displayBuffer, "%.10g", value);
}

// --- Lógica de Entrada ---

void inputDigit(char digit) {
    // Se acabamos de dar um resultado (ex: "4") e o usuário digita um número ("5"),
    // começamos uma conta nova do zero.
    if (resultJustShown) {
        strcpy(historyBuffer, "");
        strcpy(displayBuffer, "0");
        storedValue = 0;
        currentOp = 0;
        resultJustShown = 0;
        waitingForOperand = 0; // Importante resetar
    }

    if (waitingForOperand) {
        sprintf(displayBuffer, "%c", digit);
        waitingForOperand = 0;
    } else {
        if (strcmp(displayBuffer, "0") == 0) {
            sprintf(displayBuffer, "%c", digit);
        } else {
            if (strlen(displayBuffer) < 20) { 
                char temp[2] = {digit, '\0'};
                strcat(displayBuffer, temp);
            }
        }
    }
    rootRender();
}

void inputComma() {
    if (resultJustShown) {
        // Se digitar vírgula após resultado, inicia "0."
        strcpy(historyBuffer, "");
        strcpy(displayBuffer, "0.");
        storedValue = 0;
        currentOp = 0;
        resultJustShown = 0;
        waitingForOperand = 0;
    } else if (waitingForOperand) {
        strcpy(displayBuffer, "0.");
        waitingForOperand = 0;
    } else if (strchr(displayBuffer, '.') == NULL) {
        strcat(displayBuffer, ".");
    }
    rootRender();
}

// --- Operações ---

void opClearEntry() { // CE
    strcpy(displayBuffer, "0");
    rootRender();
}

void opClear() { // C
    strcpy(displayBuffer, "0");
    strcpy(historyBuffer, ""); 
    storedValue = 0;
    currentOp = 0;
    waitingForOperand = 0;
    resultJustShown = 0;
    rootRender();
}

void opBackspace() {
    if (resultJustShown) { 
        // Se tentar apagar um resultado final, limpa o histórico apenas
        strcpy(historyBuffer, "");
        return; 
    }
    if (waitingForOperand) return;
    
    int len = strlen(displayBuffer);
    if (len > 1) {
        displayBuffer[len - 1] = '\0';
    } else {
        strcpy(displayBuffer, "0");
    }
    rootRender();
}

// Operações Imediatas
void opImmediate(double (*func)(double)) {
    double val = getDisplayValue();
    double res = func(val);
    
    // Atualiza histórico para mostrar o que foi feito (ex: "sqr(9)") é complexo em C puro,
    // vamos manter simples atualizando o display.
    formatDisplay(res);
    
    waitingForOperand = 1;
    resultJustShown = 1; // Trata como resultado para permitir chain
    rootRender();
}

// Wrappers para math.h ou lógica simples
void opInvert() { 
    double val = getDisplayValue();
    if(val!=0) formatDisplay(1.0/val); else strcpy(displayBuffer, "Error");
    waitingForOperand = 1; resultJustShown = 1; rootRender();
}
void opSquare() { double val = getDisplayValue(); formatDisplay(val*val); waitingForOperand=1; resultJustShown=1; rootRender(); }
void opSqrt() { double val = getDisplayValue(); if(val>=0) formatDisplay(sqrt(val)); else strcpy(displayBuffer, "Error"); waitingForOperand=1; resultJustShown=1; rootRender(); }
void opNegate() { double val = getDisplayValue(); formatDisplay(val*-1); rootRender(); } // Negate não finaliza input
void opPercent() { double val = getDisplayValue(); formatDisplay(val/100.0); waitingForOperand=1; resultJustShown=1; rootRender(); }

// --- Lógica Matemática Principal ---

void performCalculation() {
    if (!currentOp) return;

    double currentVal = getDisplayValue();
    double result = 0;

    switch (currentOp) {
        case '+': result = storedValue + currentVal; break;
        case '-': result = storedValue - currentVal; break;
        case '*': result = storedValue * currentVal; break;
        case '/': 
            if (currentVal != 0) result = storedValue / currentVal;
            else {
                strcpy(displayBuffer, "Error");
                currentOp = 0;
                waitingForOperand = 1;
                rootRender();
                return;
            }
            break;
    }
    formatDisplay(result);
    storedValue = result;
}

void setOperator(char op) {
    // Se o usuário apertar operador depois de um resultado (ex: 2+2=4, aperta +),
    // continuamos a conta com o 4 ("4 +").
    if (resultJustShown) {
        resultJustShown = 0;
        storedValue = getDisplayValue();
    } else if (currentOp && !waitingForOperand) {
        // Se já tinha operação pendente (ex: 2+2 e aperta *), calcula o parcial
        performCalculation();
    } else {
        storedValue = getDisplayValue();
    }
    
    currentOp = op;
    
    // Atualiza histórico: "Valor Operador" (ex: "5 +")
    sprintf(historyBuffer, "%.10g %c", storedValue, currentOp);
    
    waitingForOperand = 1;
    rootRender();
}

void opEquals() {
    if (currentOp) {
        // ANTES de calcular, montamos a string completa do histórico
        // Ex: "2 + 2 ="
        double currentVal = getDisplayValue();
        sprintf(historyBuffer, "%.10g %c %.10g =", storedValue, currentOp, currentVal);

        performCalculation();
        
        currentOp = 0;
        waitingForOperand = 1;
        resultJustShown = 1; // Marca que acabamos de mostrar um resultado
        rootRender();
    }
}

// --- Wrappers de Eventos ---
void on0() { inputDigit('0'); }
void on1() { inputDigit('1'); }
void on2() { inputDigit('2'); }
void on3() { inputDigit('3'); }
void on4() { inputDigit('4'); }
void on5() { inputDigit('5'); }
void on6() { inputDigit('6'); }
void on7() { inputDigit('7'); }
void on8() { inputDigit('8'); }
void on9() { inputDigit('9'); }
void onDot() { inputComma(); }
void onAdd() { setOperator('+'); }
void onSub() { setOperator('-'); }
void onMul() { setOperator('*'); }
void onDiv() { setOperator('/'); }
void onEq()  { opEquals(); }
void onC()   { opClear(); }
void onCE()  { opClearEntry(); }
void onBack(){ opBackspace(); }
void onInv() { opInvert(); }
void onSqr() { opSquare(); }
void onSqrt(){ opSqrt(); }
void onNeg() { opNegate(); }
void onPct() { opPercent(); }

// --- UI ---

ReactComponent createBtn(char* label, void(*handler)(), char* bgColor, char* color) {
    return ReactCreateElement(
        "button",
        ReactCreateProps(
            "onClick", ReactCreateClickHandler(handler),
            "style", ReactCreateProps(
                "padding", ReactCreateString("15px"),
                "fontSize", ReactCreateString("1.1rem"),
                "border", ReactCreateString("1px solid #2d2d2d"),
                "borderRadius", ReactCreateString("4px"),
                "cursor", ReactCreateString("pointer"),
                "backgroundColor", ReactCreateString(bgColor),
                "color", ReactCreateString(color),
                "fontWeight", ReactCreateString("500")
            )
        ),
        ReactCreateString(label)
    );
}

void rootRender() {
    char* bgApp = "#202020";
    char* bgBtnNum = "#3b3b3b";
    char* bgBtnOp = "#323232";
    char* bgBtnEq = "#ff75c3";
    char* txtWhite = "#ffffff";
    char* txtBlack = "#000000";

    // Lógica de Fonte Dinâmica (Mantida da versão anterior)
    int len = strlen(displayBuffer);
    char* dynamicFontSize;
    if (len > 14) dynamicFontSize = "1.7rem";
    else if (len > 9) dynamicFontSize = "2.3rem";
    else dynamicFontSize = "3rem";

    ReactComponent app = ReactCreateElement("div", 
        ReactCreateProps(
            "style", ReactCreateProps(
                "fontFamily", ReactCreateString("'Segoe UI', sans-serif"),
                "maxWidth", ReactCreateString("320px"),
                "margin", ReactCreateString("40px auto"),
                "backgroundColor", ReactCreateString(bgApp),
                "borderRadius", ReactCreateString("8px"),
                "padding", ReactCreateString("12px"),
                "boxShadow", ReactCreateString("0 10px 30px rgba(0,0,0,0.5)"),
                "border", ReactCreateString("1px solid #444")
            )
        ),
        
        ReactCreateElement("div",
            ReactCreateProps("style", ReactCreateProps(
                "color", ReactCreateString("white"),
                "fontSize", ReactCreateString("1.2rem"),
                "fontWeight", ReactCreateString("600"),
                "padding", ReactCreateString("5px"),
                "marginBottom", ReactCreateString("5px")
            )),
            ReactCreateString("Padrão")
        ),

        // Display Area
        ReactCreateElement("div",
            ReactCreateProps(
                "style", ReactCreateProps(
                    "backgroundColor", ReactCreateString(bgApp),
                    "textAlign", ReactCreateString("right"),
                    "padding", ReactCreateString("0 5px 15px 5px"),
                    "marginBottom", ReactCreateString("5px")
                )
            ),
            
            // Histórico (Numerozinho)
            ReactCreateElement("div",
                ReactCreateProps(
                    "style", ReactCreateProps(
                        "color", ReactCreateString("#aaaaaa"), 
                        "fontSize", ReactCreateString("0.9rem"), 
                        "minHeight", ReactCreateString("1.2rem"),
                        "marginBottom", ReactCreateString("5px"),
                        "fontFamily", ReactCreateString("sans-serif"),
                        "overflow", ReactCreateString("hidden"),
                        "whiteSpace", ReactCreateString("nowrap"),
                        "textOverflow", ReactCreateString("ellipsis")
                    )
                ),
                ReactCreateString(historyBuffer)
            ),

            // Número Principal
            ReactCreateElement("div",
                ReactCreateProps(
                    "style", ReactCreateProps(
                        "color", ReactCreateString("white"),
                        "fontSize", ReactCreateString(dynamicFontSize),
                        "fontWeight", ReactCreateString("bold"),
                        "lineHeight", ReactCreateString("1"),
                        "whiteSpace", ReactCreateString("nowrap"),
                        "overflowX", ReactCreateString("auto"),
                        "overflowY", ReactCreateString("hidden"),
                        "scrollbarWidth", ReactCreateString("none")
                    )
                ),
                ReactCreateString(displayBuffer)
            )
        ),

        // Grid
        ReactCreateElement("div",
            ReactCreateProps(
                "style", ReactCreateProps(
                    "display", ReactCreateString("grid"),
                    "gridTemplateColumns", ReactCreateString("repeat(4, 1fr)"),
                    "gap", ReactCreateString("2px")
                )
            ),
            createBtn("%", onPct, bgBtnOp, txtWhite),
            createBtn("CE", onCE, bgBtnOp, txtWhite),
            createBtn("C", onC, bgBtnOp, txtWhite),
            createBtn("Del", onBack, bgBtnOp, txtWhite),
            createBtn("1/x", onInv, bgBtnOp, txtWhite),
            createBtn("x²", onSqr, bgBtnOp, txtWhite),
            createBtn("√x", onSqrt, bgBtnOp, txtWhite),
            createBtn("/", onDiv, bgBtnOp, txtWhite),
            createBtn("7", on7, bgBtnNum, txtWhite),
            createBtn("8", on8, bgBtnNum, txtWhite),
            createBtn("9", on9, bgBtnNum, txtWhite),
            createBtn("X", onMul, bgBtnOp, txtWhite),
            createBtn("4", on4, bgBtnNum, txtWhite),
            createBtn("5", on5, bgBtnNum, txtWhite),
            createBtn("6", on6, bgBtnNum, txtWhite),
            createBtn("-", onSub, bgBtnOp, txtWhite),
            createBtn("1", on1, bgBtnNum, txtWhite),
            createBtn("2", on2, bgBtnNum, txtWhite),
            createBtn("3", on3, bgBtnNum, txtWhite),
            createBtn("+", onAdd, bgBtnOp, txtWhite),
            createBtn("+/-", onNeg, bgBtnNum, txtWhite),
            createBtn("0", on0, bgBtnNum, txtWhite),
            createBtn(",", onDot, bgBtnNum, txtWhite),
            createBtn("=", onEq, bgBtnEq, txtBlack)
        )
    );

    ReactRootRender(root, app);
}

int main() {
    ReactStart();
    root = ReactDOMCreateRoot(ReactGetElementById("root"));
    rootRender();
    return 0;
}