.text
# cette directive sert a rendre l'etiquette publique
    .globl handler_IT_KEYBOARD
# debut du traitant
handler_IT_KEYBOARD:
# sauvegarde des registres importants
    pushl %eax
    pushl %edx
    pushl %ecx
# appel a la fonction C realisant le traitant
    call handler_en_C_KEYBOARD
# restauration des registres importants
    popl %ecx
    popl %edx
    popl %eax
# fin du traitant
    iret
