    .text
    .globl ctx_sw
# Structure de la pile en entree :
#   %esp + 4 : adresse de l'ancien contexte
#   %esp + 8 : adresse du nouveau contexte
ctx_sw:
    # Sauvegarde des flags du processus actuel
    pushfl                  # Sauvegarde des flags sur la pile
    popl %edx               # Récupère les flags dans edx

    # sauvegarde du contexte de l'ancien processus
    movl 4(%esp), %eax
    movl %ebx, (%eax)
    movl %esp, 4(%eax)
    movl %ebp, 8(%eax)
    movl %esi, 12(%eax)
    movl %edi, 16(%eax)
    movl %edx, 20(%eax)     # Sauvegarde des flags dans la 6ème position

    # restauration du contexte du nouveau processus
    movl 8(%esp), %eax
    movl (%eax), %ebx
    movl 8(%eax), %ebp
    movl 12(%eax), %esi
    movl 16(%eax), %edi
    movl 20(%eax), %edx     # Récupération des flags du nouveau processus

    # Préparation à la restauration des flags et esp
    movl 4(%eax), %esp      # Restauration d'ESP
    
    # Restauration des flags
    pushl %edx              # Pousse les flags sur la pile
    popfl                   # Restaure les flags
    
    # on passe la main au nouveau processus
    ret

