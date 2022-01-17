#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */

#define FILENAME_MAXLEN 8 // including the NULL char

// inode
typedef struct inode
{
    int dir; // boolean value. 1 if it's a directory.
    char name[FILENAME_MAXLEN];
    int size;         // actual file/directory size in bytes.
    int blockptrs[8]; // direct pointers to blocks containing file's content.
    int used;         // boolean value. 1 if the entry is in use.
    int rsvd;         // reserved for future use
} inode;

// directory entry
typedef struct dirent
{
    char name[FILENAME_MAXLEN];
    int namelen; // length of entry name
    int inode;   // this entry inode index
} dirent;

// Linked List
struct node
{
    struct dirent data;
    struct node *next;
};
// Prints elements of linked list
void printList(struct node *head)
{
    struct node *ptr = head;
    printf("[ ");

    while (ptr != NULL)
    {
        printf("%d(%s) ", ptr->data.inode, ptr->data.name);
        ptr = ptr->next;
    }
    printf("]\n");
}
// append element to linked list
void push(struct node **headaddr, int inode, char *name)
{
    struct node *link = (struct node *)malloc(sizeof(struct node));
    link->data.inode = inode;
    strcpy(link->data.name, name);
    link->data.namelen = strlen(name) + 1;
    link->next = NULL;

    if (*headaddr == NULL)
    {
        *headaddr = link;
    }
    else
    {
        struct node *ptr = *headaddr;
        while (ptr->next != NULL)
        {
            ptr = ptr->next;
        }
        ptr->next = link;
    }
}
// remove given inode from linked list
int delete (struct node **headaddr, int inode)
{
    struct node *current = *headaddr;
    struct node *previous = NULL;

    while (current->data.inode != inode)
    {
        if (current->next == NULL)
        {
            printf("Inode %d not in list\n", inode);
            return -1;
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }
    if (current == *headaddr)
    {
        *headaddr = current->next;
    }
    else
    {
        previous->next = current->next;
    }
    free(current);
    return 0;
}
// returns length of linked list
int length(struct node *head)
{
    int length = 0;
    struct node *current;

    for (current = head; current != NULL; current = current->next)
    {
        length++;
    }
    return length;
}
// return element of given name if exists
struct node *find(struct node *head, char *name)
{
    struct node *current = head;
    if (current == NULL)
    {
        return NULL;
    }
    else
    {
        while (strcmp(name, current->data.name) != 0)
        {
            if (current->next == NULL)
            {
                return NULL;
            }
            else
            {
                current = current->next;
            }
        }
        return current;
    }
}
// return element at given index
struct node *get(struct node *head, int index)
{
    struct node *current = head;
    if (current == NULL)
    {
        return NULL;
    }
    else
    {
        for (int i = 0; i < index; i++)
        {
            if (current->next == NULL)
            {
                return NULL;
            }
            else
            {
                current = current->next;
            }
        }
        return current;
    }
}

struct node *dataTable[127];
struct inode inodeTable[16];
int dataBitmap[127];

// Task functions

int intializeFS(); // initialize state of file system
int updateFS();    // update state of file system

// Create file
int CR(char *path, int size)
{
    if (size > 8)
    {
        printf("ERROR! Size exceeds the limit 8!\n");
        return -1;
    }
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/");

    // split the path by /
    while (token != NULL)
    {
        strcpy(arr[n], token);
        token = strtok(NULL, "/");
        n++;
    }
    int currentInode = 0; // root
    struct node *item = NULL;

    // traverse the given path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);

    // checks if target file already exists
    if (item == NULL)
    {
        i = 0;
        j = 0;
        int k = 0;

        // finds an unused inode
        while (inodeTable[i].used != 0)
        {
            i++;
            if (i == 16)
            {
                printf("ERROR! All inodes in use!\n");
                return -1;
            }
        }
        inodeTable[i].used = 1;
        inodeTable[i].dir = 0;
        strcpy(inodeTable[i].name, arr[n - 1]);
        inodeTable[i].size = size;

        // finds unused data blocks
        for (k = 0; k < size; k++)
        {
            while (dataBitmap[j] != 0)
            {
                j++;
                if (j == 127)
                {
                    printf("ERROR! Not enough space left!\n");
                    return -1;
                }
            }
            inodeTable[i].blockptrs[k] = j;
            dataBitmap[j] = 1;
        }

        // adds file to data block of parent
        push(&dataTable[inodeTable[currentInode].blockptrs[0]], i, arr[n - 1]);
        updateFS();
        return 0;
    }
    else
    {
        printf("ERROR! The file already exists!\n");
        return -1;
    }
}

// Delete file
int DL(char *path)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/");

    // splits the path by /
    while (token != NULL)
    {
        strcpy(arr[n], token);
        token = strtok(NULL, "/");
        n++;
    }
    int currentInode = 0; // root
    struct node *item = NULL;

    // traverse the path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);

    // check if target item exists and is a file
    if (item == NULL)
    {
        printf("ERROR! The file doesnot exists!\n");
        return -1;
    }
    else if (inodeTable[item->data.inode].dir == 1)
    {
        printf("ERROR! Cannot handle directories!\n");
        return -1;
    }
    else
    {
        // free up data blocks used by file
        for (i = 0; i < inodeTable[item->data.inode].size; i++)
        {
            dataBitmap[inodeTable[item->data.inode].blockptrs[i]] = 0;
        }

        // free up inode used by the file
        inodeTable[item->data.inode].used = 0;
        inodeTable[item->data.inode].size = 0;
        strcpy(inodeTable[item->data.inode].name, "");
        delete (&dataTable[inodeTable[currentInode].blockptrs[0]], item->data.inode);
        updateFS();
        return 0;
    }
}

// Copy file
int CP(char *srcpath, char *dstpath)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, srcpath);
    char *token = strtok(temp, "/");

    // split the source path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token);
            n++;
        }
        token = strtok(NULL, "/");
    }
    int currentInode = 0; // root
    struct node *item = NULL;

    // traverse the source path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);

    // check if source file exists
    if (item == NULL)
    {
        printf("ERROR! File %s not found!\n", srcpath);
        return -1;
    }
    else if (inodeTable[item->data.inode].dir == 1)
    {
        printf("ERROR! Cannot handle directories!\n");
        return -1;
    }
    else
    {
        i = 0;
        j = 0;
        n = 0;
        for (i = 0; i < 16; i++)
        {
            strcpy(arr[i], "");
        }
        strcpy(temp, dstpath);
        token = strtok(temp, "/");

        // split the destination path by /
        while (token != NULL)
        {
            if (strcmp(token, "") != 0)
            {
                strcpy(arr[n], token);
                n++;
            }
            token = strtok(NULL, "/");
        }
        int currentInode = 0; // root
        struct node *item2 = NULL;

        // traverse the destination path
        for (i = 0; i < n - 1; i++)
        {
            item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
            if (item2 == NULL)
            {
                printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
                return -1;
            }
            else
            {
                currentInode = item2->data.inode;
            }
        }

        // check if target file already exists
        item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);
        if (item2 == NULL)
        {
            i = 0;
            j = 0;
            int k = 0;

            // finds a free inode
            while (inodeTable[i].used != 0)
            {
                i++;
                if (i == 16)
                {
                    printf("ERROR! All inodes in use!\n");
                    return -1;
                }
            }
            inodeTable[i].used = 1;
            inodeTable[i].dir = 0;
            strcpy(inodeTable[i].name, arr[n - 1]);
            inodeTable[i].size = inodeTable[item->data.inode].size;

            // finds free data blocks
            for (k = 0; k < inodeTable[i].size; k++)
            {
                while (dataBitmap[j] != 0)
                {
                    j++;
                    if (j == 127)
                    {
                        printf("ERROR! Not enough space left!\n");
                        return -1;
                    }
                }
                inodeTable[i].blockptrs[k] = j;
                dataBitmap[j] = 1;
            }

            // add the file to parent data table
            push(&dataTable[inodeTable[currentInode].blockptrs[0]], i, arr[n - 1]);
            updateFS();
            return 0;
        }
        else
        {
            printf("ERROR! The file already exists!\n");
            return -1;
        }
    }
}

// Move file
int MV(char *srcpath, char *dstpath)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, srcpath);
    char *token = strtok(temp, "/");

    // split source path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token);
            n++;
        }
        token = strtok(NULL, "/");
    }
    int currentInode = 0; // root
    struct node *item = NULL;

    // traverse source path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);

    // check if source file exists
    if (item == NULL)
    {
        printf("ERROR! File %s not doesnot exists!\n", srcpath);
        return -1;
    }
    else if (inodeTable[item->data.inode].dir == 1)
    {
        printf("ERROR! Cannot handle directories!\n");
        return -1;
    }
    else
    {
        i = 0;
        j = 0;
        n = 0;
        int dataTableSrc = inodeTable[currentInode].blockptrs[0];
        for (i = 0; i < 16; i++)
        {
            strcpy(arr[i], "");
        }
        strcpy(temp, dstpath);
        token = strtok(temp, "/");

        // split the destination path by /
        while (token != NULL)
        {
            if (strcmp(token, "") != 0)
            {
                strcpy(arr[n], token);
                n++;
            }
            token = strtok(NULL, "/");
        }
        int currentInode = 0; // root
        struct node *item2 = NULL;

        // traverse the destination path
        for (i = 0; i < n - 1; i++)
        {
            item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
            if (item2 == NULL)
            {
                printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
                return -1;
            }
            else
            {
                currentInode = item2->data.inode;
            }
        }
        item2 = find(dataTable[inodeTable[currentInode].blockptrs[0]], item->data.name);

        // checks if destination file already exists
        if (item2 == NULL)
        {
            // update the inode for existing file
            push(&dataTable[inodeTable[currentInode].blockptrs[0]], item->data.inode, arr[n - 1]);
            strcpy(inodeTable[item->data.inode].name, arr[n - 1]);
            delete (&dataTable[dataTableSrc], item->data.inode);
            updateFS();
            return 0;
        }
        else
        {
            printf("ERROR! The file already exists!\n");
            return -1;
        }
    }
}

// Create directory
int CD(char *path)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/");

    // splits the path by /
    while (token != NULL)
    {
        strcpy(arr[n], token);
        token = strtok(NULL, "/");
        n++;
    }
    int currentInode = 0; // root
    struct node *item = NULL;

    // traverse the path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! %s not in directory %s!\n", arr[i], inodeTable[currentInode].name);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);

    // check if the target directory already exists
    if (item == NULL)
    {
        i = 0;
        j = 0;

        // finds an unsed inode
        while (inodeTable[i].used != 0)
        {
            i++;
            if (i == 16)
            {
                printf("ERROR! All inodes in use!\n");
                return -1;
            }
        }
        inodeTable[i].used = 1;
        inodeTable[i].dir = 1;
        strcpy(inodeTable[i].name, arr[n - 1]);
        inodeTable[i].size = 1;

        // finds unused data block
        while (dataBitmap[j] != 0)
        {
            j++;
            if (j == 127)
            {
                printf("ERROR! Not enough space left!\n");
                return -1;
            }
        }

        // set inode and data table
        inodeTable[i].blockptrs[0] = j;
        dataBitmap[j] = 1;
        push(&dataTable[j], i, ".");
        push(&dataTable[j], currentInode, "..");
        push(&dataTable[inodeTable[currentInode].blockptrs[0]], i, arr[n - 1]);
        updateFS();
        return 0;
    }
    else
    {
        printf("ERROR! Directory already exists!\n");
        return -1;
    }
}

// Delete directory
int DD(char *path)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/");

    // split the path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token);
            n++;
        }
        token = strtok(NULL, "/");
    }
    if (n == 0)
    {
        printf("ERROR! Cannot delete root directory!\n");
        return -1;
    }
    int currentInode = 0; // root
    struct node *item = NULL;

    // traverse the given path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1]);

    // check if target directiry
    if (item == NULL)
    {
        printf("ERROR! The directory doesnot exists!\n");
        return -1;
    }
    else if (inodeTable[item->data.inode].dir == 0)
    {
        printf("ERROR! Cannot handle files!\n");
    }
    else
    {
        struct node *tempNode;
        char childPath[64];
        currentInode = item->data.inode;
        int parentInode = find(dataTable[inodeTable[currentInode].blockptrs[0]], "..")->data.inode;
        while (length(dataTable[inodeTable[currentInode].blockptrs[0]]) > 0)
        {
            tempNode = get(dataTable[inodeTable[currentInode].blockptrs[0]], 0);
            if (strcmp(tempNode->data.name, ".") != 0 && strcmp(tempNode->data.name, "..") != 0)
            {
                // recursive call for sub directories
                if (inodeTable[tempNode->data.inode].dir == 1)
                {
                    strcpy(childPath, path);
                    strcat(childPath, "/");
                    strcat(childPath, tempNode->data.name);
                    DD(childPath);
                }

                // delete files inside directory
                else
                {
                    for (i = 0; i < inodeTable[tempNode->data.inode].size; i++)
                    {
                        dataBitmap[inodeTable[tempNode->data.inode].blockptrs[i]] = 0;
                    }
                    inodeTable[tempNode->data.inode].used = 0;
                    inodeTable[tempNode->data.inode].size = 0;
                    strcpy(inodeTable[tempNode->data.inode].name, "");
                    delete (&dataTable[inodeTable[currentInode].blockptrs[0]], tempNode->data.inode);
                }
            }
            else
            {
                // delete . and .. from data table
                delete (&dataTable[inodeTable[currentInode].blockptrs[0]], tempNode->data.inode);
            }
        }

        // delete inode of directory
        delete (&dataTable[inodeTable[parentInode].blockptrs[0]], currentInode);
        dataBitmap[inodeTable[currentInode].blockptrs[0]] = 0;
        inodeTable[currentInode].used = 0;
        inodeTable[currentInode].size = 0;
        strcpy(inodeTable[currentInode].name, "");
        updateFS();
        return 0;
    }
}

int LL(char *path)
{
    int i = 0, j = 0, n = 0;
    char arr[16][8];
    char temp[64];
    strcpy(temp, path);
    char *token = strtok(temp, "/");

    // split the path by /
    while (token != NULL)
    {
        if (strcmp(token, "") != 0)
        {
            strcpy(arr[n], token);
            n++;
        }
        token = strtok(NULL, "/");
    }
    int currentInode = 0; // root
    struct node *item = NULL;
    int size = 0;

    // traverse the path
    for (i = 0; i < n - 1; i++)
    {
        item = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[i]);
        if (item == NULL)
        {
            printf("ERROR! The directory %s in the given path doesnot exists!\n", arr[i]);
            return -1;
        }
        else
        {
            currentInode = item->data.inode;
        }
    }
    if (n == 0)
    {
        currentInode = 0;
    }
    else
    {
        currentInode = find(dataTable[inodeTable[currentInode].blockptrs[0]], arr[n - 1])->data.inode;
    }
    char childPath[64];
    for (i = 0; i < length(dataTable[inodeTable[currentInode].blockptrs[0]]); i++)
    {
        item = get(dataTable[inodeTable[currentInode].blockptrs[0]], i);
        if (strcmp(item->data.name, ".") != 0 && strcmp(item->data.name, "..") != 0)
        {
            if (strcmp(path, "/") != 0)
            {
                strcpy(childPath, path);
            }
            else
            {
                strcpy(childPath, "");
            }
            strcat(childPath, "/");
            strcat(childPath, item->data.name);

            // recursive call for subdirectories
            if (inodeTable[item->data.inode].dir == 1)
            {
                size += LL(childPath);
            }
            else
            {
                printf("type: file\npath: %s\nsize: %d\n\n", childPath, inodeTable[item->data.inode].size);
                size += inodeTable[item->data.inode].size;
            }
        }
    }
    size += 1; // add size of directory
    printf("type: directory\npath: %s\nsize: %d\n\n", path, size);
    return size;
}

int intializeFS()
{
    FILE *myfs = fopen("myfs.txt", "r");
    int inode;
    int dir;
    char name[FILENAME_MAXLEN];
    int size;
    int blockptrs[8];
    int dataBlockIndex;
    int flag = 1;
    int rc = 1;

    if (myfs == NULL) // initialize if file doesnot previously exists
    {
        myfs = fopen("myfs.txt", "w");
        fclose(myfs);

        inodeTable[0].used = 1;
        inodeTable[0].dir = 1;
        strcpy(inodeTable[0].name, "root");
        inodeTable[0].size = 1;
        dataBitmap[0] = 1;
        push(&dataTable[0], 0, ".");

        updateFS();
    }
    else
    {
        while (rc != EOF)
        {
            if (flag == 1) // inode table entries
            {
                rc = fscanf(myfs,
                            "%d %d %s %d %d %d %d %d %d %d %d %d",
                            &inode, &dir, name, &size, &blockptrs[0], &blockptrs[1], &blockptrs[2], &blockptrs[3], &blockptrs[4], &blockptrs[5], &blockptrs[6], &blockptrs[7]);
                if (inode == -1)
                {
                    flag = -1;
                }
                else if (rc != EOF)
                {
                    strcpy(inodeTable[inode].name, name);
                    inodeTable[inode].dir = dir;
                    inodeTable[inode].used = 1;
                    inodeTable[inode].size = size;
                    for (int i = 0; i < size; i++)
                    {
                        inodeTable[inode].blockptrs[i] = blockptrs[i];
                        dataBitmap[i] = 1;
                    }
                }
            }
            else        // data table entries
            {
                rc = fscanf(myfs,
                            "%d %s %d",
                            &dataBlockIndex, name, &inode);
                if (rc != EOF)
                {
                    push(&dataTable[dataBlockIndex], inode, name);
                }
            }
        }

        fclose(myfs);
    }
    return 0;
}

int updateFS()
{
    FILE *myfs = fopen("myfs.txt", "w");

    // inode table entries
    for (int i = 0; i < 16; i++)
    {
        if (inodeTable[i].used == 1)
        {
            fprintf(myfs, "%d %d %s %d %d %d %d %d %d %d %d %d\n", i, inodeTable[i].dir, inodeTable[i].name, inodeTable[i].size, inodeTable[i].blockptrs[0], inodeTable[i].blockptrs[1], inodeTable[i].blockptrs[2], inodeTable[i].blockptrs[3], inodeTable[i].blockptrs[4], inodeTable[i].blockptrs[5], inodeTable[i].blockptrs[6], inodeTable[i].blockptrs[7]);
        }
    }
    fprintf(myfs, "%d %d %s %d %d %d %d %d %d %d %d %d\n", -1, 0, "data", 0, 0, 0, 0, 0, 0, 0, 0, 0);   // divide

    // data table entries
    struct node *item = NULL;
    for (int i = 0; i < 16; i++)
    {
        if (inodeTable[i].used == 1 && inodeTable[i].dir == 1)
        {
            for (int j = 0; j < length(dataTable[inodeTable[i].blockptrs[0]]); j++)
            {
                item = get(dataTable[inodeTable[i].blockptrs[0]], j);
                fprintf(myfs, "%d %s %d\n", inodeTable[i].blockptrs[0], item->data.name, item->data.inode);
            }
        }
    }
    fclose(myfs);
}

// 0 for free
// 1 for used

int main(int argc, char *argv[])
{
    FILE *inpFile = fopen(argv[1], "r");
    char line[64];
    char inpCommand[3][64];
    char *token = NULL;
    int i;

    intializeFS();

    while (fgets(line, sizeof(line), inpFile))
    {
        if (line[strlen(line) - 1] == '\n')
        {
            line[strlen(line) - 1] = '\0';
        }
        i = 0;
        token = strtok(line, " ");

        // splits the input command by " "
        // command param1 param2
        while (token != NULL)
        {
            strcpy(inpCommand[i], token);
            token = strtok(NULL, " ");
            i++;
        }

        // file create
        if (strcmp(inpCommand[0], "CR") == 0)
        {
            CR(inpCommand[1], atoi(inpCommand[2]));
        }

        // file delete
        else if (strcmp(inpCommand[0], "DL") == 0)
        {
            DL(inpCommand[1]);
        }

        // file copy
        else if (strcmp(inpCommand[0], "CP") == 0)
        {
            CP(inpCommand[1], inpCommand[2]);
        }

        // file move
        else if (strcmp(inpCommand[0], "MV") == 0)
        {
            MV(inpCommand[1], inpCommand[2]);
        }

        // create directory
        else if (strcmp(inpCommand[0], "CD") == 0)
        {
            CD(inpCommand[1]);
        }

        // delete directory
        else if (strcmp(inpCommand[0], "DD") == 0)
        {
            DD(inpCommand[1]);
        }

        // list files and directories
        else if (strcmp(inpCommand[0], "LL") == 0)
        {
            LL("/");
        }
    }

    return 0;
}
