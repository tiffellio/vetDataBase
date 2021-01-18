/********************************************************************************************
                            370 FINAL PROJECT
                            Name: Tiffany Elliott
                            Please see 370Report.pdf for further details.
*********************************************************************************************/
#include <iostream>
#include <iomanip>
#include <string.h>
#include <occi.h>
#include <termios.h>
#include <unistd.h>
#include <fstream>
#include <map>
#include <ctime>
#include <sstream>
using namespace std;
using namespace oracle::occi;

// defined constants
const char Account = 'A';
const char New = 'N';
const char Schedule = 'S';
const char History = 'H';
const char Remind = 'R';
const char Exit = 'E';
const char Cancel = 'C';

// used for scheduling appointment dates
const int MAX_VALID_YR = 2030; 
const int MIN_VALID_YR = 2020; 
  
// declare an enumerated data type to enum message types
typedef enum {
    Greeting, Menu, InvalidCommand
} MSGType;

// give each prepared statement a name
struct STMT {
    string name;
    Statement *stmt;
};

/********************************** FUNCTION PROTOTYPES ***********************************/

// __________________________________ MENU AND DATABASE SETUP _____________________________
// read database password from user input
// without showing the password on the screen
string readPassword();

// displays menu using enumerated types as control
void showMessage(MSGType type);

// getCommandFromInput reads a command from the user, convert it to
//    upper-case letter and return the upper-case letter
char getCommandFromInput();

// prepared the statements that will be used repeatedly in this program
int initStatements(Connection *conn, STMT * & statements);

// Given the name, find the corresponding prepared sql statement
Statement * findStatement(string name, STMT *statements, int size);

// Terminate all the prepared statements
void terminateStatements(Connection *conn, STMT *statements, int size);

// _________________________________ I/O HELPER FUNCTIONS _________________________________

// convert an integer to a string
string intToString(int a);

// prompt user to enter a string within a character range
string getStr(string cue, int min, int max);

// checks if date is valid source for this 
// returns true if given year is valid 
bool isLeap(int year);
bool checkDate(int m, int d, int y);

// check if string is a number
// returns false if any characters are non-numbers
bool is_number(const std::string& s);

// prompts user to enter an integer within a range
int getInteger(string cue, int min, int max);

// ___________________________________ SEARCH QUERIES _________________________________________

// if valid, show a customers account details and returns true
// otherwise we will display nothing was found and return false
bool showAct(STMT *statements, int size, string actID);

// check if petID exists
bool checkPet(STMT *statements, int size, string petID);

// function to display pets info if account exists
bool showPetAct(STMT *statements, int size, string actID, bool isOwner);

// ___________________________________ MAIN TASK FUNCTIONS ___________________________________

// MAIN MENU TASK (R) - GENERATE REMINDER REPORT
// displays all pets who haven't had a check-up or visit to the vet in over a year
void showReminders(STMT *statements, int size);

// MAIN MENU TASK (H) - GENERATE MEDICAL RECORD HISTORY REPORT
// shows medical record history of a given pet including medical notes and medication history
void showMedical(STMT *statements, int size, string petID);

// MAIN MENU TASK (S) - SCHEDULE A PET IN FOR A VET APPOINTMENT
void newAppointment(STMT *statements, int size);

// MAIN MENU TASK (N) - CREATE A NEW ACCOUNT
void newAccount(STMT *statements, int size);

// MAIN MENU TASK (C) - CANCEL AN APPOINTMENT
void cancelApt(STMT *statements, int size);


int main(){
    // connect to the database
    string userName="";
    string password="";
    const string connectString = "sunfire.csci.viu.ca";

    // log into database
    cout << "Enter user name: ";
    getline(cin, userName);
    cout << "Enter password: ";
    password = readPassword();
    cout << endl;

    // set up db environment
    Environment *env = Environment::createEnvironment();
    Connection *conn = env->createConnection(userName, password, connectString);
    STMT *statements;
    int size = initStatements(conn, statements);

    string userinp;
    bool isOwner = 0;
    try{
        // show greeting 
	    showMessage(Greeting);

	    // cmd is used to store command letter
	    char cmd;

	    // print the initial message and command options
	    showMessage(Menu);
	    bool shutdown = 0;
	    string actID = "";

        // main menu functionality
	    while (!shutdown) {
	        cmd = getCommandFromInput(); 

            // look up a an owner or pets account
	        if (cmd == Account) {
                    // prompt user to enter account
                do {
                    userinp = getStr("Please enter the owner ID to show all pets or the pet ID: ", 6, 6);
                } while(!is_number(userinp));

                // if account is an owner account, display owner and all pet accounts
                if(showAct(statements,size, userinp)){
                    isOwner = 1;
                    if(!showPetAct(statements,size, userinp, isOwner)){
                        cout << "No pets under this account.\n";
                    }
                // if account was not found under the owner, show pet account if found
                } else {
                    // account number wasn't found in user or pet tables
                    if(!showPetAct(statements,size, userinp, isOwner)){
                        cout << "Nothing found under this number.\n";
                    }
                }
                isOwner = 0; // reset boolean
			
            // create new account (owner & pet)
            } else if (cmd == New) {
                newAccount(statements, size);

            // schedule an appointment
			} else if (cmd == Schedule) { 
                newAppointment(statements, size);

            // for medical history
			} else if (cmd == History) {
                // prompt user to enter account, it can be owner or pet
                userinp = getStr("Please enter the customer's Account ID to search OR Pet ID: ", 6, 6);
                // if account is an owner account, display owner and all pets accounts
                if(showAct(statements, size, userinp)){
                    isOwner = 1;
                    if(!showPetAct(statements,size, userinp, isOwner)){
                        cout << "No pets under this account.\n";
                    } else {
                        userinp = getStr("Enter the pet ID you would like to generate a report of: ", 6, 6);
                        showMedical(statements, size, userinp);
                    }
                // if account was not found under the owner, show pet account if found
                } else {
                    // account number wasn't found in user or pet tables
                    if(!checkPet(statements, size, userinp)){
                        cout << "Nothing found under this number.\n";
                    } else {
                        //pet account found, generate report
                        showMedical(statements, size, userinp);
                    }
                }
                isOwner = 0; // reset boolean

            // look up reminder contacts
			} else if (cmd == Remind) {
	            showReminders(statements, size);

            // cancel an appointment
			} else if (cmd == Cancel) {
                cancelApt(statements, size);

            // end the program
            } else if (cmd == Exit) {
				shutdown = 1;

            // unknown command   
	        } else {
	            showMessage(InvalidCommand);
	        }
	    }    
	} catch (SQLException & e) {
        cout << e.what();
        // clean up environment before terminating
        terminateStatements(conn, statements, size);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        return 0;
    }

    // clean up environment before terminating
    terminateStatements(conn, statements, size);
    env->terminateConnection(conn);
    Environment::terminateEnvironment(env);
    return 0;

} // end of main

// __________________________________ MENU AND DATABASE SETUP ____________________________
// read database password from user input
// without showing the password on the screen
string readPassword() {
    struct termios settings;
    tcgetattr( STDIN_FILENO, &settings );
    settings.c_lflag =  (settings.c_lflag & ~(ECHO));
    tcsetattr( STDIN_FILENO, TCSANOW, &settings );

    string password = "";
    getline(cin, password);

    settings.c_lflag = (settings.c_lflag |   ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &settings );
    return password;
}

// displays menu using enumerated types as control
void showMessage(MSGType type) {
    switch (type) {
       case Greeting:
           cout << endl
                << "*** Welcome to Welcome to the VI Vet Clinic System ***"
                << endl << endl;
           break;
       case Menu:
           cout << endl
                << "Enter 'A' to view accounts" << endl
                << "   or 'N' to add a new customer" << endl
                << "   or 'S' to schedule an appointment" << endl
                << "   or 'C' to cancel an appointment" << endl
                << "   or 'H' to generate a medical history report" << endl
                << "   or 'R' to search for customer reminder contacts" << endl
                << "   or 'E' to exit this program" << endl
                << endl;
           break;
       case InvalidCommand:
           cout << endl
                << "Invalid command." << endl
                << endl;
           break;
    }
}
// getCommandFromInput reads a command from the user, convert it to
//    upper-case letter and return the upper-case letter
char getCommandFromInput() {
    char cmd;
    string garbage;
  
    cout << "-----------------------------------" << endl;
    cout << "Please enter your command: ";
    cin >> cmd;
    getline(cin, garbage);
  
    return toupper(cmd);
}

//___________________________________________ QUERIES ________________________________________________________
// prepared the statements that will be used repeatedly in this program
int initStatements(Connection *conn, STMT * & statements) {
    int size = 13; // set to number of queries
    statements = new STMT[size];

    // query to look up owner's account
    statements[0].name = "checkAct";
    string qry = qry + "Select OwnerID, Name, Email, Address from Owners where OwnerID = :1";
    statements[0].stmt = conn->createStatement(qry);

    // query to look up pet's account
    statements[1].name = "showPetAct";
    qry = "Select PetID, OwnerID, PetName, Age, Species, Colour, Weight from Pets where PetID = :1";
    statements[1].stmt = conn->createStatement(qry);

    // query to look up pets under owner's account
    statements[2].name = "ownersPets";
    qry = "Select PetID, OwnerID, PetName, Age, Species, Colour, Weight from Pets where OwnerID = :1";
    statements[2].stmt = conn->createStatement(qry);

    // query to look up pets who haven't had a medical record (implying no visit) in over 12 months
    statements[3].name = "checkReminder";
    qry = "SELECT O.Name, P.PetName, O.Email, O.Address, O.OwnerID FROM Owners O, Pets P, Appointments A \n \
           WHERE O.OwnerID = P.OwnerID AND P.PetID = A.PetID AND A.AptDate <= ADD_MONTHS(TRUNC(SYSDATE), -12)";
    statements[3].stmt = conn->createStatement(qry);

    // query to look up pets information and medical record notes
    statements[4].name = "checkRecord";
    qry = "SELECT P.PetName, P.Age, P.Species, P.Colour, P.Weight, M.RecordID, M.Notes, P.PetID \n \
            FROM Pets P, MedicalRecords M WHERE P.PetID = M.PetID AND P.PetID = :1";
    statements[4].stmt = conn->createStatement(qry);

    // query to look up pets information and prescription medication record notes
    statements[5].name = "checkMed";
    qry = "SELECT P.PetID, PR.Rx, PR.Refill, PR.ExpiryDate, PR.Price FROM MedicalRecords M, \n \
    Contains C, Prescriptions PR, Pets P \n \
    WHERE P.PetID = M.PetID AND M.RecordID = C.RecordID AND C.Rx = PR.Rx AND P.PetID = :1 ";
    statements[5].stmt = conn->createStatement(qry);

    // query to create a new appointment
    statements[6].name = "checkApt";
    qry = "INSERT INTO appointments(aptid, petid, aptdate, starttime, duration) \n \
    VALUES((select max(aptid) from appointments) + 1,:1,to_date(:2,'mm/dd/yyyy'), to_timestamp(:3, 'HH24:MI'),:4)"; 
    statements[6].stmt = conn->createStatement(qry);

    // query to create a new owner account
    statements[7].name = "newOwner";
    qry = "INSERT INTO Owners(OwnerID, Name, Email, Address) \n \
     VALUES((SELECT MAX(OwnerID) from Owners) + 1, :1, :2, :3)"; 
    statements[7].stmt = conn->createStatement(qry);

    // query to retreive newest owner account (use as a foreign key for creating pet account)
    statements[8].name = "findOwner";
    qry = "Select MAX(OwnerID) From Owners"; 
    statements[8].stmt = conn->createStatement(qry);

    // query to create a new pet account
    statements[9].name = "newPet";
    qry = "INSERT INTO Pets(PetID, OwnerID, PetName, Age, Species, Colour, Weight) \n \
    VALUES((SELECT MAX(PETID) FROM Pets) + 1, :1, :2, :3, :4, :5, :6)"; 
    statements[9].stmt = conn->createStatement(qry);

    // query to search appointment by petid
    statements[10].name = "petApt";
    qry = "SELECT A.AptID, A.AptDate, A.StartTime, A.Duration FROM Appointments A, \n \
    Pets P WHERE P.PetID = A.PetID AND P.PetID = :1"; 
    statements[10].stmt = conn->createStatement(qry);

    // query to verify valid appointment ID was entered
    statements[11].name = "srchApt";
    qry = "SELECT * from Appointments WHERE AptID = :1"; 
    statements[11].stmt = conn->createStatement(qry);

    // query to delete appointment by ID w
    statements[12].name = "dltApt";
    qry = "DELETE FROM Appointments WHERE AptID = :1"; 
    statements[12].stmt = conn->createStatement(qry);

    return size;
}

// Given the name, find the corresponding prepared sql statement
Statement * findStatement(string name, STMT *statements, int size) {
    for(int i = 0; i < size; i++) {
        if (statements[i].name == name)
            return statements[i].stmt;
    }
    return 0;
}

// Terminate all the prepared statements
void terminateStatements(Connection *conn, STMT *statements, int size) {
    for(int i = 0; i < size; i++)
        conn->terminateStatement(statements[i].stmt);
}

// _________________________________ I/O HELPER FUNCTIONS ________________________________

// convert an integer to a string
string intToString(int a) {
    ostringstream temp;
    temp << a;

    return temp.str();
}

// prompt user to enter a string within a character range
string getStr(string cue, int min, int max) {
    cout << cue;
    string str;
    getline(cin, str);

    while (str.length() < min || str.length() > max){
        cout << "Value must be between " << min << " and " << max << " characters." << endl;
        cout << "Please try again" << endl;
        getline(cin, str);
    }

    return str;
}

// checks if date is valid. source for this function:
// https://www.geeksforgeeks.org/program-check-date-valid-not/
bool isLeap(int year){ 
    // return true if year is a multiple pf 4 and 
    // not multiple of 100. OR year is multiple of 400. 
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)); 
} 

bool checkDate(int m, int d, int y){
    // if year, month and day are not in given range 
    if (y > MAX_VALID_YR || y < MIN_VALID_YR){ 
        cout << "Year out of range.\n";
        return false; 
    }

    if (m < 1 || m > 12) {
        cout << "Month out of range.\n";
        return false; 
    }

    if (d < 1 || d > 31) {
        cout << "Day out of range.\n";
        return false; 
    }
  
    // handle February month with leap year 
    if (m == 2) { 
        if (isLeap(y)) 
        return (d <= 29); 
        else
        return (d <= 28); 
    } 
  
    // months of April, June, Sept and Nov must have  
    // number of days less than or equal to 30 
    if (m == 4 || m == 6 || m == 9 || m == 11) 
        return (d <= 30); 
    return true; 

}

// check if string is a number
// returns false if any characters are non-numbers
bool is_number(const std::string& s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
} 

// prompts user to enter an integer within a range
int getInteger(string cue, int min, int max) {
    int num;
    string inp;
    bool isValid = 0;
    cout << cue;

    do{
        cin >> num;
        if (cin.fail() || num < min || num > max) {
            cout << "Please enter an integer in the range "<< min <<" - "<< max <<": ";
            cin.clear();
            getline(cin, inp);
        } else {
            getline(cin, inp);
            isValid = 1;
        }
    } while (!isValid); // keep asking until it is valid

    return num;
}

// check if petID exists
bool checkPet(STMT *statements, int size, string petID) {
    Statement *stmt = findStatement("showPetAct", statements, size);
    stmt->setString(1, petID);
    ResultSet *rs = stmt->executeQuery();

    if(rs->next()){
        cout << "pet found\n";
        return true;
    } else return false;
    stmt->closeResultSet(rs);    
}

// function to display pets info if account exists
bool showPetAct(STMT *statements, int size, string actID, bool isOwner) {
    Statement *stmt;

    // if an owner ID was entered, we want to display all owners pets
    if (isOwner == 0){
        stmt = findStatement("showPetAct", statements, size);
        stmt->setString(1, actID);

    // if it wasn't an owner ID, try showing pet
    } else {
        stmt = findStatement("ownersPets", statements, size);
        stmt->setString(1, actID);      
    }

    ResultSet *rs = stmt->executeQuery();
    int petCount = 1;

    if (rs->next()){
        do {
            cout << "PET " << petCount << ":\n";
            cout << "*  Pet ID: " << rs->getString(1) << endl;
            cout << "*  Pet's Name: " << rs->getString(3) << endl;
            cout << "*  Pet's Age: " << rs->getString(4) << endl;
            cout << "*  Species/Type: " << rs->getString(5) << endl;
            cout << "*  Colour: " << rs->getString(6) << endl;
            cout << "*  Weight(lbs): " << rs->getString(7) << endl << endl;
            petCount ++;
         } while (rs->next());
     }else{
         return false;
     }
    stmt->closeResultSet(rs);
    return true;
}

// ___________________________________ MAIN TASK FUNCTIONS ___________________________________

// MAIN MENU TASK (A) - VIEW ACCOUNT
// if valid, show a customers account details and returns true
// otherwise we will display nothing was found and return false
bool showAct(STMT *statements, int size, string actID){
    Statement *stmt = findStatement("checkAct", statements, size);
    stmt->setString(1, actID);
    ResultSet *rs = stmt->executeQuery();

    if (rs->next()){
        do {
            cout << "*************************************************"<< endl;
            cout << "*    Account ID: " << rs->getString(1) << endl;
            cout << "*     Full Name: " << rs->getString(2) << endl;
            cout << "* Email Address: " << rs->getString(3) << endl;
            cout << "*       Address: " << rs->getString(4) << endl;
            cout << "*************************************************"<< endl;
            cout << "\t **" << rs->getString(2) << "'s Pets **" << endl << endl;
         } while (rs->next());
         return true;
    }else{
         return false;
    }

    stmt->closeResultSet(rs);
}

// MAIN MENU TASK (R) - GENERATE REMINDERS
// displays all pets who haven't had a check-up or visit to the vet in over a year
void showReminders(STMT *statements, int size) {
    Statement *stmt = findStatement("checkReminder", statements, size);
    ResultSet *rs = stmt->executeQuery();

    int petCount = 1;
    string inp = "";
    ofstream fout;

    // run reminder report and display on screen
    if (rs->next()){
        do {
            cout << rs->getString(2) << "'s owner, "    << rs->getString(1) 
            << ", needs a check-up reminder" << endl;   
            cout << "CONTACT INFO:" << endl;
            cout << "* Email: " << rs->getString(3) << endl;
            cout << "* Mailing Address: " << rs->getString(4) << endl;
            cout << "* Account ID: " << rs->getString(5) << endl; 
            cout << "**********************************************************************" << endl << endl;
            petCount ++;
        } while (rs->next());

        // allow the option to generate a printable report
        cout << "Would you like to print a contact-sheet for " << petCount << " patients? (y/n)" << endl;
        getline(cin, inp);

        if(inp == "y" || inp == "Y"){
            // grab current date for report name
            time_t t = time(NULL);
            tm* timePtr = localtime(&t);
            int day = timePtr->tm_mday;
            int mon = timePtr->tm_mon;
            int yr = timePtr->tm_year + 1900;
            string sDay = intToString(day);
            string sMon = intToString(mon);
            string sYr = intToString(yr);

            // open file
            string filename = "Report" + sMon + "-" + sDay + "-" + sYr + ".txt"; 
            fout.open(filename.c_str());

            // if failed, generate error message
            if (fout.fail()) {
                cout << "Failed to open that file." << endl;
                return;
            }
            // run again and output to report file
            rs = stmt->executeQuery();
            if (rs->next()){
                do {
                    fout << rs->getString(2) << "'s owner, "    << rs->getString(1) << ", needs a check-up reminder" << endl;   
                    fout << "CONTACT INFO:" << endl;
                    fout << "* Email: " << rs->getString(3) << endl;
                    fout << "* Mailing Address: " << rs->getString(4) << endl;
                    fout << "* Account ID: " << rs->getString(5) << endl; 
                    fout << "**********************************************************************" << endl << endl;
                } while (rs->next());
               fout.close(); 
               cout << "Report generated." << endl;
            }
        }
    }else{
        cout << "No pets are found under this account ID." << endl;
    }
    stmt->closeResultSet(rs);
}

// MAIN MENU TASK (H)  - GENERATE MEDICAL RECORD HISTORY REPORT
// shows medical record history of a given pet including medical notes and medication history
void showMedical(STMT *statements, int size, string petID) {
    Statement *stmt = findStatement("checkRecord", statements, size);
    stmt->setString(1, petID);
    ResultSet *rs = stmt->executeQuery();

    // display record
    if (rs->next()){
        cout << "\t\t\t MEDICAL HISTORY \t\t\n";
        cout << "***************************** Patient " << rs->getString(8) << " **********************\n";
        cout << "*  Pet Name: " << rs->getString(1) << endl;
        cout << "*  Age: " << rs->getString(2) << endl;
        cout << "*  Species/Type: " << rs->getString(3) << endl;
        cout << "*  Fur Color Description: " << rs->getString(4) << endl;
        cout << "*  Weight(lbs): " << rs->getString(5) << "lbs" << endl;
        cout << "**********************************************************************\n\n\n";
        cout << "* MEDICAL NOTES: " << rs->getString(7) << endl;
        cout << "* Record No. " << rs->getString(6) << endl << endl;

        // keep displaying medical notes         
        while(rs->next()){
            cout << "*  MEDICAL NOTES: " << rs->getString(7) << endl;
            cout << "* Record No. " << rs->getString(6) << endl;
        }
    }else{
         cout << "No records are found." << endl;
    }
    stmt->closeResultSet(rs);

    // prescription report
    stmt = findStatement("checkMed", statements, size);
    stmt->setString(1, petID);
    rs = stmt->executeQuery();

    // display record
    if(rs -> next()){
        do {
            cout << "\t\t\t PRESCRIPTION HISTORY \t\t\n";
            cout << "****************************** Patient " << rs->getString(1) << " **************************\n";
            cout << "*  Rx #: " << rs->getString(2) << endl;
            cout << "*  Refill #: " << rs->getString(3) << endl;
            cout << "*  Expiration Date: " << rs->getString(4) << endl;
            cout << "*  $" << rs->getString(5) << " (CAD)" << endl;
            cout << "******************************************************************************\n\n\n";
        } while (rs -> next());
    }else{
         cout << "No records are found." << endl;
    }
    stmt->closeResultSet(rs);
}

// MAIN MENU TASK (S) - SCHEDULE A PET IN FOR A VET APPOINTMENT
void newAppointment(STMT *statements, int size) {
    int mm, dd, yyyy, hh, mi, len;
    int err = 0;
    string petID = "";
    string mon, day, year, hour, min, length;
    string date = "";
    string time = "";
    Statement *stmt = findStatement("checkApt", statements, size);
    cout << "SCHEDULE A NEW APPOINTMENT\n";

    // gather petID, keep looping until valid number is entered
    bool isOwner = 0;
    do{
        if (err != 0) cout << "Pet ID not found, please try again, or press 0 to exit\n";
        petID = getStr("Enter the Pet's ID: \n", 6, 6);
        err ++;
    } while((!showPetAct(statements,size, petID, isOwner)) || (!is_number));
    err = 0;
    stmt->setString(1, petID);

    // gather appointment date, keep looping until a valid date is entered
    do {
        if (err !=0) cout << "Please try again.\n";
        mm = getInteger("Enter the month (mm): ", 1, 12);
        dd = getInteger("Enter the day (dd): ", 1, 31);
        yyyy = getInteger("Enter the year (yyyy): ", MIN_VALID_YR, MAX_VALID_YR);
        err ++;
    } while(!checkDate(mm,dd,yyyy));
    err = 0;

    // convert date into mm/dd/yyyy format
    mon = intToString(mm);
    day = intToString(dd);
    year = intToString(yyyy);
    date = date + mon + "/" + day + "/" + year;
    cout << date << endl;
    stmt->setString(2, date);

    // get the appointment start time 
    hh = getInteger("Enter the start time hour (h): ", 0, 24);
    mi = getInteger("Enter the start time minutes (m): ", 1, 31);

    // convert time into HH:MI (24 hr) format
    hour = intToString(hh);
    min = intToString(mi);
    string start = "";
    start = start + hour +":" + min;
    cout << start << endl;
    stmt->setString(3, start);

    // get the length of appointment
    len = getInteger("Enter the length of appointment in minutes.\n", 1, 1000);
    length = intToString(len);
    stmt->setString(4, length);

    // create appointment
    int ret = stmt->executeUpdate();
    if(ret = 0){
        cout << "Failed to create appointment for " << petID << endl;
    } else {
        cout << "Appointment at " << date << " successfully created!\n";
    }
}

// MAIN MENU TASK (N) - CREATE A NEW ACCOUNT
void newAccount(STMT *statements, int size) {
    cout << "***************************** CREATE A NEW ACCOUNT *******************************\n";
    cout << "********** OWNER PROFILE - Please enter primary owner *********"; 
    
    string email, name, address;
    int err =0;

    // gather owner field information
    do {
        if(err != 0) cout << "Must be a non number. Try again.\n";
        err ++;
        name = getStr("\nOwner Name (First Last): ", 3, 100);
    } while (is_number(name));
    err = 0;
    email = getStr("\nEmail: ", 5, 150);
    address = getStr("\nStreet Address: ", 5, 200);

    // create owner
    Statement *stmt = findStatement("newOwner", statements, size);
    stmt->setString(1, name);
    stmt->setString(2, email); 
    stmt->setString(3, address);
    int ret = stmt->executeUpdate();

    // let user know status
    if(ret != 0){
        cout << "Owner account created.\n";
    } else {
        cout << "Failed to create owner account.\n";   
    }

    // store the new ownerID to use as pet foreign key
    stmt = findStatement("findOwner", statements, size); 
    ResultSet *rs = stmt->executeQuery();
    string ownerid = "";
    if (rs -> next()){
        ownerid = rs -> getString(1);
    } else {
        cout << "Failed to find owner ID to create pet account.\n";
        return;
    }   
    stmt->closeResultSet(rs);

    // get pet(s) information
    string petName, age, species, color, weight;
    cout << "* PET PROFILE "; 
    int petCount = getInteger("\n* How many pets do they have? ", 1, 20);

    // gather all fields ensuring correct format before inserting
    for(int i = 1; i <= petCount; i++){
        cout << "\n ENTER PET #" << i << "\n";
        do {
            if(err != 0) cout << "Must be a non number. Try again.\n";
            err ++;
            petName = getStr("\n* Pet name: ", 1, 100);
        } while (is_number(petName));
        err = 0;

        do {
            if(err != 0) cout << "Must be a number. Try again.\n";
            err ++;
            age = getStr("\n* Age: ", 1, 3); // integer length 1-3
        } while (!is_number(age));
        err = 0;

        do {
            if(err != 0) cout << "Must be a non number. Try again.\n";
            err ++;
            species = getStr("\n* Species/Types: ", 1, 100);
        } while (is_number(species));
        err = 0;

        do {
            if(err != 0) cout << "Must be a non number. Try again.\n";
            err ++;
            color = getStr("\n* Color Description: ", 1, 100);    
        } while (is_number(color));
        err = 0;

        do{
            if(err != 0) cout << "Must be a number. Try again.\n";
            err ++;
            weight = getStr("\n* Weight (lbs): ", 1, 4); // integer length 1-4
        } while (!is_number(weight));
        err = 0;

        // attempt to insert into database
        stmt = findStatement("newPet", statements, size);  
        stmt->setString(1, ownerid);
        stmt->setString(2, petName); 
        stmt->setString(3, age);
        stmt->setString(4, species);
        stmt->setString(5, color);
        stmt->setString(6, weight);
        ret = stmt->executeUpdate();

        // let user know status
        if(ret != 0){
            cout << "Pet account created.\n";
        } else {
            cout << "Failed to create owner account.\n";   
        }
    }
}

// MAIN MENU TASK (C) - CANCEL AN APPOINTMENT
void cancelApt(STMT *statements, int size) {
    // look up appointments by pets id
    string PetID = getStr("Please enter the Pet's ID that you cancel or 000000 to exit this menu: ", 6, 6);
    if (PetID == "000000") return;
    Statement *stmt = findStatement("petApt", statements, size);
    stmt->setString(1, PetID);
    ResultSet *rs = stmt->executeQuery();

    // display appointments found for that pet to cancel, or return if not found
    string AptID, AptDate, Start, Duration, usrApt, usrInp;
    int err = 0;

    // let user see all appointments to pick which one to cancel
    if(rs -> next()){
        do{
            AptID = rs -> getString(1);
            AptDate = rs -> getString(2);
            Start = rs -> getString(3);
            Duration = rs -> getString(4);
            cout << "Appointment found for this pet: \n";
            cout << "\t Appointment ID: " << AptID << endl;
            cout << "\t Appointment Date: " << AptDate << endl;
            cout << "\t Appointment Start Time: " << Start << endl;
            cout << "\t Appointment Duration: " << Duration << endl << endl;
        } while (rs -> next());
        stmt->closeResultSet(rs);

        // get the users appointment ID, ensuring it is correct first
        do{
            if(err != 0) cout << "Please try again. It has to be numerical.\n";
            usrApt = getStr("Enter the Appointment ID of the appointment you would like to cancel: ", 10, 10);      
        } while (!is_number);

        // once a properly-formatted ID is acquired, search appointment database
        bool isValid = 0;
        stmt = findStatement("srchApt", statements, size);

        // get an appointment ID from user that is valid
        do{
            stmt->setString(1, usrApt);
            rs = stmt->executeQuery();
            if(rs-> next()){
                isValid = 1;
            } else {
                cout << "Not a valid ID, please try again. \n";
                usrApt = getStr("Enter the Appointment ID of the appointment you would like to cancel: ", 10, 10); 
            }
        } while (!isValid);
        stmt->closeResultSet(rs);

        // if confirmed, delete the appointment 
        if(isValid){
            cout << "\n\n **CONFIRM CANCELLATION: Are you sure you want to delete Appointment #" << usrApt << " ? (y/n)**";
            getline(cin, usrInp);
            if(usrInp == "y" || usrInp == "Y"){

                //delete the appointment from the database
                stmt = findStatement("dltApt", statements, size);
                stmt->setString(1, usrApt);
                rs = stmt->executeQuery();
                stmt->closeResultSet(rs);
                cout << "Appointment successfully cancelled. \n";
            } else {
                cout << "Appointment not cancelled.\n";
            }
        } else {
            cout << "Something went wrong. Cannot cancel that appointment.\n";
            return;
        }
    // no results for that query
    } else {
        cout << "There are no appointments found for that pet. \n";
        return;
    }
}
