int ElementalTranslationUnit1(void);
int ElementalTranslationUnit2(void);
int ElementalToolsTranslationUnit1(void);
int ElementalToolsTranslationUnit2(void);

int main(void)
{
    return ElementalTranslationUnit1() +
           ElementalTranslationUnit2() +
           ElementalToolsTranslationUnit1() +
           ElementalToolsTranslationUnit2();
}
