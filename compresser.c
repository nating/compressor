//
//  compresser.c
//  
//
//  Created by Geoffrey Natin on 06/12/2015.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <wchar.h>
#define ASCIICHARACTERS 256
#define MAXCODESIZE 32

//--------------------------------------------------------STRUCTS---------------------------------------------------------//

struct bitfile{
    FILE * file;
    unsigned char buffer;
    int index;
};

struct bitfile * newBitfile(char * filename, char * mode){
    struct bitfile * result;
    result = malloc(sizeof(struct bitfile));
    result->file = fopen(filename,mode);
    result->buffer = strcmp(mode,"r")==0? fgetc(result->file) : 0;
    result->index = 0;
    return result;
}

//A node with a code in a Huffman tree.
struct symbol{
    int freq;
    int isLeaf;
    char code[MAXCODESIZE];
    int codeLength;
    union{
        struct{
            struct symbol * left;
            struct symbol * right;
        } compound;
        unsigned char c;
    } u;
};

//Constructor for a non-leaf symbol.
struct symbol * newCompoundSymbol(struct symbol * left,struct symbol * right){
    struct symbol * newSymbol;
    newSymbol = malloc(sizeof(struct symbol));
    newSymbol->isLeaf = 0;
    newSymbol->freq = (left->freq + right->freq);
    newSymbol->u.compound.left = left;
    newSymbol->u.compound.right = right;
    return newSymbol;
}

//Constructor for a leaf symbol.
struct symbol * newSymbol(unsigned char i){
    struct symbol * newSymbol;
    newSymbol = malloc(sizeof(struct symbol));
    newSymbol->isLeaf = 1;
    newSymbol->freq = 0;
    newSymbol->u.c = i;
    return newSymbol;
}

//---------------------------------------------------------FILE.METHODS-----------------------------------------------------//

//Write to a bitfile
void bitfilePrint(struct bitfile * this, char value){
    int v = (value=='0')? 0 : 1;
    this->buffer = v? (this->buffer | v<<(this->index)) : this->buffer;
    this->index++;
    if(this->index==8){
        fputc(this->buffer,this->file);
        this->buffer = 0;
        this->index = 0;
    }
}

//Read from a bitfile
int bitfileRead(struct bitfile * this){
    int value = ((this->buffer & (1<<(this->index)))>>(this->index));
    this->index++;
    if(this->index==8){
        this->buffer = fgetc(this->file);
        this->index = 0;
    }
    return value;
}

//Check if a bitfile is at the EOF
int bitfileEOF(struct bitfile * b){
    return feof(b->file);
}

//Close a bitfile
void bitfileClose(struct bitfile * b){
    fclose(b->file);
    free(b);
}

//Encode File into a compressed file
void encodeInput(FILE * inputFile, char * destFileName, struct symbol ** codes){
    struct bitfile * out = newBitfile(destFileName,"w");
    int c;
    c = fgetc(inputFile);
    while(!feof(inputFile)){
        for(int i=0;i<codes[c]->codeLength;i++){
            bitfilePrint(out,codes[c]->code[i]);
        }
        c = fgetc(inputFile);
    }
    fputc(out->buffer,out->file);
    bitfileClose(out);
}

//Decode compressed file
void decodeInput(char * srcFileName, char * destFileName, struct symbol * huffmanTree){
    struct bitfile * inputFile = newBitfile(srcFileName,"r");
    struct symbol * p = huffmanTree;
    FILE * out = fopen(destFileName,"w");
    assert(out!=NULL);
    
    //Read every character from the buffer except when there is an EOF character in the file.
    while(!bitfileEOF(inputFile)){
        if(p!=huffmanTree){ fputc(p->u.c,out); p = huffmanTree; }
        while(p->isLeaf==0){
            p = bitfileRead(inputFile)? p->u.compound.right : p->u.compound.left;
        }
    }
    bitfileClose(inputFile);
    fclose(out);
}

//------------------------------------------------------HUFFMAN.METHODS----------------------------------------------------------//

//Remove from a minimum priority queue
struct symbol * removeMin(struct symbol **s, int queueSize){
    struct symbol * min = s[0];
    s[0] = s[queueSize-1];
    struct symbol * temp;
    int sinker = 0;
    int smallest = sinker;
    int left;
    int right;
    while(sinker<=(queueSize-1)/2){
        left = sinker*2+1;
        right = sinker*2+2;
        if(left<queueSize-1 && s[left]->freq<s[smallest]->freq){
            smallest = left;
        }
        if(right<queueSize-1 && s[right]->freq<s[smallest]->freq){
            smallest = right;
        }
        if(sinker==smallest){
            sinker = queueSize;
        }
        else{
            temp = s[smallest];
            s[smallest] = s[sinker];
            s[sinker] = temp;
            sinker = smallest;
        }
    }
    return min;
}

//Add to a Priority Queue (only one return value; the new priority queue. Size must be updated in calling function)
struct symbol ** addToPQ(struct symbol ** s,int queueSize, struct symbol * item){
    struct symbol * temp;
    s[queueSize] = item;
    int j = queueSize;
    while(s[j]->freq<s[(j-1)/2]->freq){
        temp = s[(j-1)/2];
        s[(j-1)/2] = s[j];
        s[j] = temp;
        j = (j-1)/2;
    }
    return s;
}

//Set the huffman codes of the symbols of a huffman tree
void setCodes(struct symbol *s,char * code,int length){
    if(s==NULL){ printf("NULL passed to method 'printCodes()'\n"); }
    if((s->isLeaf)==0){
        length++;
        strcat(code,"0");
        setCodes(s->u.compound.left,code,length);
        code[strlen(code)-1] = '1';
        setCodes(s->u.compound.right,code,length);
        code[strlen(code)-1] = '\0';
    }
    else{
        strcpy(s->code,code);
        s->codeLength = length;
    }
}

//Free the memory of all Compound Nodes
void freeCompounds(struct symbol *s){
    if(s==NULL){ printf("NULL passed to method 'freeCompounds()'\n"); }
    if((s->isLeaf)==0){
        freeCompounds(s->u.compound.left);
        freeCompounds(s->u.compound.right);
        free(s);
    }
}

//Sets Huffman codes of an array of symbols and returns the root of a huffman tree
struct symbol * setHuffmanCodes(struct symbol **characters){
    int queueLength = ASCIICHARACTERS;
    char code[MAXCODESIZE] = "0";
    struct symbol *newCompound;
    struct symbol *minFreq1;
    struct symbol *minFreq2;
    
    //Copy pointers so the array is in its original order from the calling Function
    struct symbol ** characters2 = malloc(ASCIICHARACTERS*sizeof(struct symbol));
    for(int i=0;i<ASCIICHARACTERS;i++){
        characters2[i] = characters[i];
    }
    
    //Turn array into minimum priority queue
    for(int i=ASCIICHARACTERS/2;i>=0;i--){
        int sinker = i;
        int left;
        int right;
        int smallest = sinker;
        while(sinker<=(ASCIICHARACTERS-1)/2){
            left = sinker*2+1;
            right = sinker*2+2;
            if(left<ASCIICHARACTERS && characters2[left]->freq<characters2[smallest]->freq){
                smallest = left;
            }
            if(right<ASCIICHARACTERS && characters2[right]->freq<characters2[smallest]->freq){
                smallest = right;
            }
            if(sinker==smallest){
                sinker = ASCIICHARACTERS;
            }
            else{
                newCompound = characters2[smallest];
                characters2[smallest] = characters2[sinker];
                characters2[sinker] = newCompound;
                sinker = smallest;
            }
        }
    }
    
    //Compound the symbols of the least frequency that are not leaves or yet to be compounded
    while(queueLength>1){
        minFreq1 = removeMin(characters2,queueLength);
        queueLength--;
        minFreq2 = removeMin(characters2,queueLength);
        queueLength--;
        newCompound = newCompoundSymbol(minFreq1,minFreq2);
        characters2 = addToPQ(characters2,queueLength,newCompound);
        queueLength++;
    }
    minFreq1 = removeMin(characters2,queueLength);
    
    //Traverse the left then right branch of the root setting codes of the leaves
    setCodes(minFreq1->u.compound.left,code,1);
    strcpy(code,"1");
    setCodes(minFreq1->u.compound.right,code,1);
    
    //Return the root of the huffman tree
    return minFreq1;
}

//---------------------------------------------------------MAIN------------------------------------------------------------//

int main(int argc, char ** argv)
{
    if(argc<5){fprintf(stderr, "Useage: huffman <filename>\n(Not enough arguments)\n");exit(1);}
    else if(argc>5){fprintf(stderr, "Useage: huffman <filename>\n(Too many arguments)\n");exit(1);}
    
    unsigned char c;
    char * control;
    FILE * trainingFile;
    struct symbol ** characters;
    struct symbol * huffmanTreeRoot;
    characters = malloc(ASCIICHARACTERS*sizeof(struct symbol));
    for(int i=0;i<ASCIICHARACTERS;i++){
        characters[i] = newSymbol(i);
    }
    
    //Extract characters from training file text
    trainingFile = fopen(argv[2], "r");
    assert(trainingFile != NULL );
    c = fgetc(trainingFile);
    while(!feof(trainingFile)){
        characters[c]->freq++;
        c = fgetc(trainingFile);
    }
    fclose(trainingFile);
    
    //Change the frequencies of characters not present in the text to 1
    for(int i=0;i<ASCIICHARACTERS;i++){
        if(characters[i]->freq==0){
            characters[i]->freq = characters[i]->freq + 1;
        }
    }
    
    //Compute Huffman codes for array for coding
    huffmanTreeRoot = setHuffmanCodes(characters);
    
    //Compress file
    if(strcmp(argv[1],"huffcode")==0){
        FILE * input;
        input = fopen(argv[3],"r");
        assert(input!=NULL);
        encodeInput(input,argv[4],characters);
    }
    //Decompress file
    else if(strcmp(argv[1],"huffdecode")==0){
        decodeInput(argv[3],argv[4],huffmanTreeRoot);
    }
    
    freeCompounds(huffmanTreeRoot);
    for(int i=0;i<ASCIICHARACTERS;i++){
        free(characters[i]);
    }
    free(characters);
    
    return 0;
};
