#include <iostream>
#include <string> 
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <thread>
#include <future>

//Namespaces in my C++ code? get outta here
//I'm sure I'll regret this eventually
//The end user can't see my terrible code once it's compiled *taps head*
using namespace std;

//Calculate the chi squared of the input text, must be lowercase and only a-z
double fitness(string text)
{
    string alph = "abcdefghijklmnopqrstuvwxyz";

    //Count the occurances of each letter in the text and keep track in the occurances array
    vector<int> occurrences;
    for (int i = 0; i < 26; i++)
    {
        int tmp_occur = 0;
        string::size_type start = 0;

        while ((start = text.find(alph[i], start)) != string::npos)
        {
            ++tmp_occur;
            start += 1;
        }
        occurrences.push_back(tmp_occur);
    }

    //Now work out the percetanges for each letter
    vector<double> freq;
    int total_num = accumulate(occurrences.begin(), occurrences.end(), 0);
    for (int i = 0; i < 26; i++)
    {
        freq.push_back(occurrences[i] / (double)total_num);
    }

    //Actual letter frequencies from A-Z (sum to 1.0)
    vector<double> expected =
    {
        0.0813, 0.0149, 0.0271, 0.0432, 0.1202, 0.023, 0.0203, 0.0592, 0.0731,
        0.001, 0.0069, 0.0398, 0.0261, 0.0695, 0.0768, 0.0182, 0.0011, 0.0602,
        0.0628, 0.091, 0.0288, 0.0111, 0.0209, 0.0017, 0.0211, 0.0007
    };

    //Then calculate the actual chi squared value using these
    double result = 0;
    for (int i = 0; i < 26; i++)
    {
        double chi_top = pow((freq[i] - expected[i]), 2);
        double chi_bottom = expected[i];
        result += chi_top / chi_bottom;
    }

    return result;
}



//Decode the hill cipher rows when used as a child thread process
void rowDecodeList(promise<std::vector<std::pair<vector<int>, double>>> p, vector<vector<int>> keyList, vector<vector<int>> ct_chunked)
{
    string alph = "abcdefghijklmnopqrstuvwxyz";

    //I.e. do we need to muck around with keeping a limited list size to conserve memory?
    int memory_limit = 0;
    if (keyList.size() > 676)
    {
       memory_limit = 1;
    }


    //Calculate the temp row plaintexts
    //For each row permutation get 1 letter from each ciphertext chunk and add it to a temp ciphertext
    std::vector<std::pair<vector<int>, double>> ordered_sub_list; // Will hold the final results
    for (int i = 0; i < keyList.size(); i++)
    {
        vector<int>row_perm = keyList[i];  //Get the current row permutation

        string temp_plaintext;
        for (int j = 0; j < ct_chunked.size(); j++)
        {
            vector<int>ct_chunk = ct_chunked[j];    //Get the current ct_chunk

            //Calculate the dot product between the row perm and current ct chunk
            int chunk_num = (inner_product(ct_chunk.begin(), ct_chunk.end(), row_perm.begin(), 0)) % 26;

            //Convert it into a letter and add it to the temp plaintext

            temp_plaintext += alph[chunk_num];
        }

        //Now we've got the full temp plaintext we can calculate its chi squared score
        //
        double chi_score = fitness(temp_plaintext);


        double worst_score = 9999999;
        //If we do need to conserve memory only keep a list of the top 30 results
        //May be an off by 1 error here
        if (memory_limit == 1)
        {
            //If our list isn't full then just add the permutations to it anyway
            if (i < 30)
            {
                ordered_sub_list.push_back(make_pair(row_perm, chi_score));
            }

            //At the end of this grace period we then sort the list to get the starting worst value
            if (i == 30)
            {
                //Sort them lowest score first for the best row results
                std::sort(ordered_sub_list.begin(), ordered_sub_list.end(),
                    [](const auto& i, const auto& j) { return i.second < j.second; });
                worst_score = ordered_sub_list[29].second;
            }


            //If the chi score is better than the worst in our current list then replace the last element with it
            //and sort the list again
            if (i > 30 && chi_score < worst_score)
            {
                ordered_sub_list[29] = make_pair(row_perm, chi_score);

                //Sort them lowest score first for the best row results
                std::sort(ordered_sub_list.begin(), ordered_sub_list.end(),
                    [](const auto& i, const auto& j) { return i.second < j.second; });
            }

        }   //End of memory management branch

        else    //Otherwise we can just add all possible permutations to a list
        {
            ordered_sub_list.push_back(make_pair(row_perm, chi_score));
        }

    }

    p.set_value(ordered_sub_list);   //What we actually return via the promise
}



int main(int argc, char* argv[])
{
    //Global alph
    string alph = "abcdefghijklmnopqrstuvwxyz";

    //Set placeholder user inputs to see if they're actually changed later
    string ciphertext = "PLACEHOLDER_CT";
    int thread_count = 1337;
    int N = 1337;
    vector<int> user_selected_rows;

    //Parse the arguments for actual inputs
    if (argc == 1)  //If no flags were given
    {
        cout << "\nERROR, correct usage:\n\n"
            "hillBrute.exe -t integer -n integer -c ciphertext -p {optional permutation}\n\n"
            "-t : threadcount to use\n"
            "-n : what nxn matrix to check\n"
            "-c : ciphertext to decode\n"
            "-p : optionally pick which of the top rows to try instead of the default top N. Input as -p 2,5,3,N\n";
    }


    //For each argument given
    for (int i = 1; i < argc; ++i)
    {
        string currentArg(argv[i]);
        string tmp_perm_int;
        string old_tmp_perm_int;    //For the last entry

        //If the current argument is actually a flag
        if (currentArg[0] == '-')
        {
            string currentNextArg(argv[i + 1]);
            switch (currentArg[1])  //Check which flag
            {
            case 't':   //Set threads
                thread_count = (int)*(argv[i + 1]) - 48;  //(Ascii so -48 to get correct range)
                break;
            case 'c':   //Set ciphertext
                ciphertext = string(argv[i + 1]);
                break;

            case 'n':   //Set N matrix size
                N = (int)*(argv[i + 1]) - 48;
                break;

            case 'p':    //Set which rows to use
                //Split the argument on commas, convert the string to int and add it to user_selected_rows
                for (int j = 0; j < currentNextArg.length(); j++)
                {
                    if (currentNextArg[j] != ',')
                    {
                        tmp_perm_int += currentNextArg[j];
                    }

                    else
                    {
                        user_selected_rows.push_back(stoi(tmp_perm_int)-1);
                        tmp_perm_int = "";
                    }
                    old_tmp_perm_int = tmp_perm_int;
                }
                user_selected_rows.push_back(stoi(old_tmp_perm_int)-1);   //Deal with the last input that doesn't end in ,

                break;

            default:    //RTFM
                cout << "\nERROR, correct usage:\n\n"
                    "hillBrute.exe -t integer -n integer -c ciphertext -p {optional permutation}\n\n"
                    "-t : threadcount to use\n"
                    "-n : what nxn matrix to check\n"
                    "-c : ciphertext to decode\n"
                    "-p : optionally pick which of the top rows to try instead of the default top N. Input as -p 2,5,3,N\n";

            }
        }
    }

    //Sanity check to make sure the user actually input all the needed values:
    if (ciphertext == "PLACEHOLDER_CT" || N == 1337 || thread_count == 1337)
    {
        return 1;
    }

    //Print status
    cout << "Starting...\n";

    //Convert the ciphertext into an a0z25 list
    vector<int> ciphertext_a0_z25;
    for (int i = 0; i < ciphertext.length(); i++)
    {
        ciphertext_a0_z25.push_back((int)ciphertext[i] - 97);
    }

    //Convert this a0z25 list into chunks of N length
    vector<vector<int>> ct_chunked;
    int chunk_remainder = ciphertext_a0_z25.size() % N;  //How many leftover items do we have

    //Deal with the whole items first
    for (int i = 0; i < floor(ciphertext_a0_z25.size() / N); i++)
    {
        ct_chunked.push_back(vector<int>(ciphertext_a0_z25.begin() + N * i, ciphertext_a0_z25.begin() + (N * i) + N));
    }

    //Deal with any leftover items that wouldn't fit as a whole N chunk
    //Only runs if there actually is a leftover chunk
    if (ciphertext_a0_z25.size() % N != 0)
    {
        ct_chunked.push_back(vector<int>(ciphertext_a0_z25.begin() + floor(ciphertext_a0_z25.size() / N) * N, ciphertext_a0_z25.end()));
    }



    //Make all permutations of rows of length N to test
    //For ex [0,0,0], [0,0,1], [0,0,2] ... [0,0,25], [0,1,0] ... [25,25,25]
    vector<vector<int>> row_permutations;
    for (int i = 0; i < pow(26, N); i++)
    {
        //Make this temp vector which will have N elements and add each element to it before adding it to the permutation proper list:
        vector<int>temp_perm_vector;
        for (int j = 0; j < N; j++)
        {
            temp_perm_vector.push_back((int)(floor(i / pow(26, j))) % 26);
        }
        row_permutations.push_back(temp_perm_vector);
    }

    cout << "Finished permutation generation\n";



    //Calculate the temp row plaintexts
    //For each row permutation get 1 letter from each ciphertext chunk and add it to a temp ciphertext

    //Split up the row_permutations into t chunks for the number of threads we're going to use
    //Convert this a0z25 list into chunks of N length for each thread to deal with
    vector<vector<vector<int>>> row_permutations_chunked;
    int chunk_size = floor(row_permutations.size() / thread_count);

    //Deal with the whole items first
    //Deal with the first thread_count - 1 chunks first to avoid any excess overflow issues
    for (int i = 0; i < thread_count - 1; i++)
    {

        row_permutations_chunked.push_back(vector<vector<int>>(row_permutations.begin() + chunk_size * i, row_permutations.begin() + (chunk_size * i) + chunk_size));
    }
    //Deal with any leftover items here
    row_permutations_chunked.push_back(vector<vector<int>>(row_permutations.begin() + chunk_size * (thread_count - 1), row_permutations.end()));
    //split end



    //Multi-threading temp perm plaintexts begins here
    std::vector<std::future<std::vector<std::pair<vector<int>, double>>>> futureResults;
    std::vector<std::thread> threadPool;

    for (int i = 0; i < thread_count; i++)
    {
        //Keep track of the return promises
        promise<std::vector<std::pair<vector<int>, double>>> promise;
        futureResults.push_back(promise.get_future());

        // Create the thread, pass it its promise and keep track in the thread pool.
        threadPool.push_back(thread(rowDecodeList, move(promise), row_permutations_chunked[i], ct_chunked));
    }


    //Actually join all threads
    for (int i = 0; i < thread_count; i++)
    {
        threadPool[i].join();
    }


    //Get results from all threads
    //Now we need to calculate each temp plaintext's chi squared score and sort them
    std::vector<std::pair<vector<int>, double>> ordered_list;

    for (int i = 0; i < thread_count; i++)
    {
        std::vector<std::pair<vector<int>, double>> b = futureResults[i].get();
        ordered_list.insert(ordered_list.end(), b.begin(), b.end());
    }

    cout << "Finished temp row plaintexts\n";



    //Sort them lowest score first for the best row results
    std::sort(ordered_list.begin(), ordered_list.end(),
        [](const auto& i, const auto& j) { return i.second < j.second; });



    //Print the top N*3 rows with their chi squared values just in case the top N ones aren't the correct ones
    cout << "\nTop N*3 rows: \n";
    for (int i = 0; i < N * 3; i++)
    {
        cout << i+1 << ": " << "{";
        for (int j = 0; j < N - 1; j++) //Print each part of whatever permutation we're on
        {
            cout << ordered_list[i].first[j] << ", ";
        }
        cout << ordered_list[i].first[N - 1];   //Leave this till last so we can format the , nicely

        //End the permutation and print the chi squared score
        cout << "} Chi squared: " << ordered_list[i].second << "\n";
    }
    cout << "\n\n";


    //Take the top N rows and find all permutations of them
    vector<vector<int>> top_rows;

    //If the user hasn't specified the rows to use from the arguments then just pick
    //The top N rows
    if (user_selected_rows.size() == 0)
    {
        for (int i = 0; i < N; i++)
        {
            top_rows.push_back(ordered_list[i].first);
        }
    }

    //Otherwise pick the user intended rows
    else
    {
        for (int i = 0; i < N; i++)
        {
            top_rows.push_back(ordered_list[user_selected_rows[i]].first);
        }


    }



    //We need to first sort the rows so it starts at the lowest permutation and actually cycles through them all during next_permutation
    std::sort(top_rows.begin(), top_rows.end(),
        [](const auto& i, const auto& j) { return i < j; });

    //Actually calculate all the permutations of theese top N rows
    vector<vector<vector<int>>> inv_key_matrix_perms;
    do
    {
        inv_key_matrix_perms.push_back(top_rows);
    } while (std::next_permutation(top_rows.begin(), top_rows.end()));



    //Now work out the actual plaintexts for all these permutations of the presumed proper rows
    cout << "Possible plaintexts:\n";
    for (int i = 0; i < inv_key_matrix_perms.size(); i++)
    {
        vector<vector<int>>current_inv_key_perms = inv_key_matrix_perms[i];

        string temp_plaintext;
        for (int j = 0; j < ct_chunked.size(); j++)
        {
            vector<int>ct_chunk = ct_chunked[j];    //Get the current ct_chunk
            vector<int> chunk_num;  //Will hold the result of the matrix multi. between ct_chunk and whatever matrix perm we're on

            for (int k = 0; k < N; k++) //For each row of the matrix work out the dot product for the full mult. of the matricies
            {
                chunk_num.push_back(inner_product(ct_chunk.begin(), ct_chunk.end(), current_inv_key_perms[k].begin(), 0) % 26);
            }

            //We then convert these chunk_num numbers from a0z25 into plaintext and add it to the temp_plaintext and keep doing it till we've got the full thing
            for (int k = 0; k < N; k++) //For each of the chunk numbers
            {
                temp_plaintext += alph[chunk_num[k]];
            }
        }

        //Print the result, one of these should be the actual plaintext
        cout << (i + 1) << "\n" << temp_plaintext << "\n\n";

    }

    //Finished proper
    cout << "Finished\n";
    return 1;
}
