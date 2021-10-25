from multiprocessing import Pool
import numpy as np
import itertools
import math

##Implementing https://alexbarter.com/cryptanalysis/breaking-hill-cipher/

#Define functions
#Split a list up into chunkNum number of lists, seems to use less memory than np.split but slower
def splitChunks(arrayIn, chunkNum):
    fullArray = []
    chunkNum = (len(arrayIn)//chunkNum)
    chunkSize = len(arrayIn)//chunkNum

    for i in range(0,chunkNum-1):
        start = i*chunkSize
        fullArray.append(arrayIn[start:start+chunkSize])

    fullArray.append(arrayIn[(chunkNum-1)*chunkSize::])
    return fullArray

def fitness(text):  #Calc with chi squared monogram comparison
    text = (text.replace(" ","")).lower()
    alph = "abcdefghijklmnopqrstuvwxyz"
    ocrr = np.zeros(26)
    freq = []
    for i in text:  #Add each letter to a total count array
        if(alph.find(i)!=-1):
            ocrr[alph.find(i)]+=1
    total = sum(ocrr)
    for i in range(0,len(ocrr)):    #Calculate letter percentages
        freq.append(ocrr[int(i)]/total)

    #####CHI SQUARED#####
    expected = [8.13,1.49,2.71,4.32,12.02,2.30,2.03,5.92,7.31,0.10,0.69
                ,3.98,2.61,6.95,7.68,1.82,0.11,6.02,6.28,9.10,2.88,1.11
                ,2.09,0.17,2.11,0.07]
    expected = np.divide(expected,100)
    chi = 0
    for i in range(0,len(freq)):
        chitop = (freq[i]-expected[i])**2
        chibottom= expected[i]
        
        chi+=(chitop/chibottom)
    return chi

def fitnessList(textList):  #For use with multiprocess, does fitness on a list of texts
    fullList = []

    for i in range(len(textList)):    #Add chi values to each possibility
        fullList.append([textList[i][1],fitness(textList[i][0].lower())])

    return fullList

#Convert a=0, b=1, c=2 in a list
def strToNum(ct_str):
    alph = "abcdefghijklmnopqrstuvwxyz"
    ct_num_list = []
    
    for char in ct_str:
        ct_num_list.append(alph.find(char))

    return ct_num_list

#For use with multiprocess, calculate vig plaintext for the entire given list
def rowDecodeList(keyList):
    #For each row permutation get 1 letter from each ciphertext chunk and add it to a temp ciphertext
    temp_plaintext_sub_list = []
    for row_perm in keyList:
        temp_plaintext = ""
        for ct_chunk in ct_chunked:
            chunk_num = np.dot(ct_chunk, row_perm) % 26
            temp_plaintext += alph[chunk_num] #Convert the number to a letter
            
        temp_plaintext_sub_list.append([temp_plaintext, row_perm])

    return temp_plaintext_sub_list


######USER INPUT HERE####################################
alph = "abcdefghijklmnopqrstuvwxyz"
ciphertext = """iqiputrjutgckfyjqyqzkgcovfpejhcxngnirswplekqmblsklvwxlirchsdvzoihajluwjdaiitdryoaejjunuchpjluxdxztdaponzvevdaupsppuzdeqrttoshxuseniuyrmeekhiacntcwmuoebicrtccfodtqrpiqdwclbnupnneoualnauoqkfgajdnmaobwwuixsbqbppyuskkgjipkhaycajdnmaobwgkwtazpyughdtwegjihvyvaopysafxjpwztdapohtjhvpzkcfdezvytdyrtgeiyrjuzbtgtpiayoltmhygjihtjvugvsoypzckihpnwnfpsdouazhmcjybrzasyhgppaxvkciuwgnpvegulpgptfeluoegnihnezxeckierkgnicjmtrsazvufjqzzzkhzauqsgovkoauvqafrbphjyxznwoplxghvukarfjbvkchanxafkokyqlpfhcvwpbxzhzkhsvvjrfevqnjvxopeprugiazrxkhrjuyowxxhvbmckiazrfnbttpckikliwfcfclzvcawkxvwzaasmmgwvgzvbyuxsghtjncavrxjgqsdjdivzqgjyqkkhntwuoevoupvyhzcjdkovkqfvthvntdpsuthxvmdtqobiwmbnjesvbadqobecwrknjvwxuwlnzthsiaafxgavwndyeqvbajawrpyvulchwhduwzbunzgfjshpwxuoievbmwigejbeqrvomjesmmuqzwawuizfhdsfuyqxrqmkicjzvgatekzxtdcjehgnifnzuictrydiwibdaycdpptdlzolztbdgdanzluqhpwwjdcyysdjqvbhvcemusuymhyhpgjqswmfdxmgqgyphckiyowljlvmtxlidtddmxascznxyownugnjvvrgncgjqywzlrjuwqukpruictrytrqizmgpdafxlpmymenwvdzmcjhvlagnihdsgaiinhokizqrrjnrnobtgjyxrjspfyaupygquljvxicrsebhkpdxpdjlqrqwbmeuoewnhynkvisqrsqdnjeskjyreytwgtrqeqjwndvskjlqthsfuciyivoatznvafexjjehtqrrsephqcmebzhyykeffbzdczhnxkgygdtqwnhcuklllawgfvwwnhbzbnjdfzafdjxnomvcbmajgflzuzvyxkuctrteduciihsiayeffckkdhezojvgpefftygnyownhenbhlogznedvvpwjlqpsozoiqjj"""
N = 3
threads = 6
######USER INPUT END####################################





#Convert the ciphertext to a a0z25 list
ciphertext_numbers = strToNum(ciphertext)

#Convert that list into a list of chunks of N
ct_chunked = splitChunks(ciphertext_numbers, N)

#Only run if main process, not multithread child:
if __name__ == '__main__':
    
    #Make all permutations of rows of length N to test
    row_permutations = list(itertools.product(range(26), repeat=N))
    
    #We split the list of combinations up into chunks for the number of processes we're using
    sub_key_lists = splitChunks(row_permutations, threads)

    #Calculate temp row plaintexts with multithreadings
    print("Calculating row temp plaintexts...")
    with Pool(threads) as p:
        results = (p.map(rowDecodeList, sub_key_lists))

    #Combine each thread's sublist results once they're finished
    temp_plaintext_list = [item for sublist in results for item in sublist]

    #We then calculate the fitness of each and sort with chi squared
    print("Calculating fitness...")

    #We split the list of combinations up into chunks for the number of processes we're using
    comboChunks = splitChunks(temp_plaintext_list, threads)

    with Pool(threads) as p:
        results = (p.map(fitnessList, comboChunks))

      
    orderedlist = [item for sublist in results for item in sublist] #Combine each thread's sublist
    orderedlist = sorted(orderedlist, key=lambda chi: chi[1])   #Sort list for lowest chi score

    ####Output results####
    print("\nTop results:")
    for i in range(0,N*5):
        print(orderedlist[i][0], orderedlist[i][1])
        
    print("\nTrying all permutations of top N rows:")
    ##Now we assume the top N results are the valid key rows (this may not be true) and encode all permutation of them to check the resultant proper plaintext:
    top_rows = []
    for i in range(0,N):
        top_rows.append(orderedlist[i][0])

    #Make all permutations of top rows:
    inv_key_matrix_perms = itertools.permutations(top_rows)

    #Print the final plaintexts from the top N rows
    for inv_key_matrix in inv_key_matrix_perms:
        temp_plaintext = ""
        for ct_chunk in ct_chunked:
            chunk_num = np.matmul(inv_key_matrix,ct_chunk) % 26
            for j in range(0,len(chunk_num)):
                temp_plaintext += alph[chunk_num[j]] #Convert the number to a letter
        print(temp_plaintext, inv_key_matrix, "\n")
