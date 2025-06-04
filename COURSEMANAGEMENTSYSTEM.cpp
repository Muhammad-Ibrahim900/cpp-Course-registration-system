#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <limits>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;


class Person {
public:
    string surname;
    string firstname;
    string email;

    Person() = default;
    Person(const string& first, const string& last, const string& mail)
        : firstname(first), surname(last), email(mail) {}

    virtual string getDetails() const {
        return firstname + " " + surname + " (" + email + ")";
    }

    virtual void displayInfo() const = 0;
    virtual ~Person() = default;
};


class RegistrationException : public exception {
    string message;
public:
    explicit RegistrationException(const string& msg) : message("Registration Error: " + msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class FileIOException : public exception {
    string message;
public:
    explicit FileIOException(const string& msg) : message("File I/O Error: " + msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};


class Lecturer : public Person {
public:
    string academic_title;

    Lecturer() = default;

    Lecturer(const string& first, const string& last, const string& mail, const string& title)
        : Person(first, last, mail), academic_title(title) {}

    void displayInfo() const override {
        cout << *this << endl;
    }

    friend ostream& operator<<(ostream& os, const Lecturer& l) {
        os << l.academic_title << " " << l.firstname << " " << l.surname
           << " (" << l.email << ")";
        return os;
    }
};


class Student : public Person {
public:
    int matriculation_number = 0;
    string university;
    vector<string> enrolled_courses;

    Student() = default;

    Student(const string& first, const string& last, const string& mail, int matric_num, const string& uni)
        : Person(first, last, mail), matriculation_number(matric_num), university(uni) {}

    void displayInfo() const override {
        cout << *this << endl;
    }

    friend ostream& operator<<(ostream& os, const Student& s) {
        os << s.firstname << " " << s.surname << ", Matric#: " << s.matriculation_number
           << ", University: " << s.university << ", Email: " << s.email
           << ", Enrolled courses: ";
        for (size_t i = 0; i < s.enrolled_courses.size(); ++i) {
            os << s.enrolled_courses[i];
            if (i < s.enrolled_courses.size() - 1) os << ", ";
        }
        return os;
    }
};

class Course {
public:
    string course_name;
    Person* lecturer;               // polymorphism 
    vector<Person*> participants;  

    Course() : lecturer(nullptr) {}
    Course(string name, Person* lec) : course_name(std::move(name)), lecturer(lec) {}

    void display() const {
        cout << *this << endl;
    }

    bool isFullyBooked() const {
        return participants.size() >= 10;
    }

    friend ostream& operator<<(ostream& os, const Course& c) {
        os << "Course: " << c.course_name << "\nLecturer: ";
        if (c.lecturer)
            c.lecturer->displayInfo();
        else
            os << "None\n";

        os << "Participants:\n";
        if (c.participants.empty()) {
            os << " None\n";
        } else {
            for (const auto* p : c.participants) {
                p->displayInfo();
            }
        }
        if (c.participants.size() < 3) {
            os << "Course will not take place (less than 3 participants).\n";
        }
        return os;
    }
};


map<string, Student*> allStudents;     
vector<Person*> allPeople;              


const string dataFolder = "data/";
const string studentsFile = dataFolder + "students.txt";
const string coursesFile = dataFolder + "courses.txt";


void handleInvalidInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Invalid input. Please enter a valid number." << endl;
}


void saveStudents() {
    try {
        fs::create_directories(dataFolder);
        ofstream out(studentsFile);
        if (!out) throw FileIOException("Cannot open " + studentsFile);

        for (const auto& [email, student] : allStudents) {
            out << student->firstname << "|" << student->surname << "|" << student->email << "|"
                << student->matriculation_number << "|" << student->university << "|";
            for (const auto& course : student->enrolled_courses) {
                out << course << ",";
            }
            out << "\n";
        }
    } catch (const FileIOException& e) {
        cerr << e.what() << endl;
    }
}

void loadStudents() {
    ifstream in(studentsFile);
    if (!in) return;

    string line;
    while (getline(in, line)) {
        size_t pos = 0;
        vector<string> parts;

        while ((pos = line.find('|')) != string::npos) {
            parts.push_back(line.substr(0, pos));
            line.erase(0, pos + 1);
        }
        if (parts.size() < 5) continue; 

        auto* student = new Student(parts[0], parts[1], parts[2], stoi(parts[3]), parts[4]);

        size_t cpos = 0;
        while ((cpos = line.find(',')) != string::npos) {
            string course = line.substr(0, cpos);
            if (!course.empty()) student->enrolled_courses.push_back(course);
            line.erase(0, cpos + 1);
        }

        allStudents[student->email] = student;
        allPeople.push_back(student);
    }
}


class CourseCatalog {
private:
    vector<Course> courses;

    CourseCatalog() = default;
    CourseCatalog(const CourseCatalog&) = delete;
    CourseCatalog& operator=(const CourseCatalog&) = delete;

    const string coursesFile = dataFolder + "courses.txt";

public:
    static CourseCatalog& getInstance() {
        static CourseCatalog instance;
        return instance;
    }

    void addCourse(const Course& course) {
        courses.push_back(course);
    }

    Course& getCourse(size_t index) {
        if (index >= courses.size()) throw out_of_range("Invalid course index");
        return courses[index];
    }

    const vector<Course>& getCourses() const {
        return courses;
    }

    void displayNotFullyBookedCourses() const {
        cout << "Courses with free places:" << endl;
        for (const auto& c : courses) {
            int free_places = 10 - int(c.participants.size());
            if (free_places > 0) {
                cout << c.course_name << ": " << free_places << " free places, Lecturer: ";
                c.lecturer->displayInfo();
            }
        }
    }

    void saveCourses() {
        try {
            fs::create_directories(dataFolder);
            ofstream out(coursesFile);
            if (!out) throw FileIOException("Cannot open " + coursesFile);

            for (const auto& course : courses) {
                Lecturer* lecPtr = dynamic_cast<Lecturer*>(course.lecturer);
                if (!lecPtr) continue;  

                out << course.course_name << "|" << lecPtr->firstname << "|"
                    << lecPtr->surname << "|" << lecPtr->email << "|" << lecPtr->academic_title << "|";

                for (const auto* p : course.participants) {
                    out << p->email << ",";
                }
                out << "\n";
            }
        } catch (const FileIOException& e) {
            cerr << e.what() << endl;
        }
    }

    void loadCourses() {
        ifstream in(coursesFile);
        if (!in) return;

        string line;
        while (getline(in, line)) {
            size_t pos = 0;
            vector<string> parts;

            while ((pos = line.find('|')) != string::npos) {
                parts.push_back(line.substr(0, pos));
                line.erase(0, pos + 1);
            }

            if (parts.size() < 5) continue;

            
            Person* lecturerPtr = nullptr;
            bool foundLecturer = false;
            for (Person* p : allPeople) {
                if (p->email == parts[3]) {
                    lecturerPtr = p;
                    foundLecturer = true;
                    break;
                }
            }
            if (!foundLecturer) {
                lecturerPtr = new Lecturer(parts[1], parts[2], parts[3], parts[4]);
                allPeople.push_back(lecturerPtr);
            }

            Course course(parts[0], lecturerPtr);

            size_t cpos = 0;
            while ((cpos = line.find(',')) != string::npos) {
                string email = line.substr(0, cpos);
                if (allStudents.count(email)) {
                    course.participants.push_back(allStudents[email]);
                }
                line.erase(0, cpos + 1);
            }

            courses.push_back(course);
        }
    }
};

void registerStudentToCourse() {
    CourseCatalog& catalog = CourseCatalog::getInstance();

    int courseIndex;
    cout << "Select course (1: Programming, 2: Databases, 3: Software Engineering): ";
    if (!(cin >> courseIndex) || courseIndex < 1 || courseIndex > 3) {
        handleInvalidInput();
        return;
    }

    Course& course = catalog.getCourse(courseIndex - 1);
    if (course.isFullyBooked()) {
        cout << "Course is fully booked." << endl;
        return;
    }

    string firstname, surname, email, university;
    int matriculation_number;

    cout << "First name: "; cin >> firstname;
    cout << "Last name: "; cin >> surname;
    cout << "Email: "; cin >> email;
    cout << "Matriculation number: ";
    while (!(cin >> matriculation_number)) {
        handleInvalidInput();
    }
    cout << "University (Enter 'BAHRIA' if you're from BAHRIA): ";
    cin >> university;

    
    if (allStudents.count(email)) {
        Student* existingStudent = allStudents[email];
       
        if ((university == "BU" && existingStudent->enrolled_courses.size() >= 3) ||
            (university != "BU" && existingStudent->enrolled_courses.size() >= 1)) {
            throw RegistrationException("Enrollment limit reached for student with email " + email);
        }
        
        existingStudent->enrolled_courses.push_back(course.course_name);
        course.participants.push_back(existingStudent);
        cout << "Successfully registered for " << course.course_name << "." << endl;
    } else {
        
        Student* newStudent = new Student(firstname, surname, email, matriculation_number, university);
        newStudent->enrolled_courses.push_back(course.course_name);
        allStudents[email] = newStudent;
        allPeople.push_back(newStudent);
        course.participants.push_back(newStudent);
        cout << "Successfully registered for " << course.course_name << "." << endl;
    }

    saveStudents();
    catalog.saveCourses();
}

void displayAllEnrollments() {
    cout << "Enrolled courses for each student:" << endl;
    for (const auto& [email, student] : allStudents) {
        cout << student->firstname << " " << student->surname << ": ";
        for (size_t i = 0; i < student->enrolled_courses.size(); ++i) {
            cout << student->enrolled_courses[i];
            if (i < student->enrolled_courses.size() - 1) cout << ", ";
        }
        cout << endl;
    }
}

int main()
 {
    
    Lecturer* lec1 = new Lecturer("Adil", "Khan", "adilkhan@gmail.com", "Professor");
    Lecturer* lec2 = new Lecturer("Nabia", "Khalid", "nabiakhalid@gmail.com", "Associate Professor");
    Lecturer* lec3 = new Lecturer("Sohail", "Akhtar", "sohailakhtar@gmail.com", "Researcher");

    allPeople.push_back(lec1);
    allPeople.push_back(lec2);
    allPeople.push_back(lec3);

    
    CourseCatalog& catalog = CourseCatalog::getInstance();
    catalog.addCourse(Course("Programming", lec1));
    catalog.addCourse(Course("Databases", lec2));
    catalog.addCourse(Course("Software Engineering", lec3));

    loadStudents();
    catalog.loadCourses();

    int choice;
    cout << "----:Welcome to The Bahria University (BU):---\n";
    cout << "At BU university, 3 courses are offered:\n\"Programming\", \"Databases\", and \"Software Engineering\".\n";

    do {
        cout << "\nMenu:\n1. Register for a course\n2. Output course details\n3. Output not fully booked courses\n4. Display enrolled courses for each student\n5. End program\nEnter choice: ";
        if (!(cin >> choice)) {
            handleInvalidInput();
            continue;
        }

        try {
            switch (choice) {
                case 1:
                    registerStudentToCourse();
                    break;

                case 2: {
                    int c;
                    cout << "Enter course number (1-3): ";
                    if (!(cin >> c) || c < 1 || c > 3) {
                        handleInvalidInput();
                        break;
                    }
                    catalog.getCourse(c - 1).display();
                    break;
                }

                case 3:
                    catalog.displayNotFullyBookedCourses();
                    break;

                case 4:
                    displayAllEnrollments();
                    break;

                case 5:
                    cout << "\nCourses not taking place due to low participation (<3 participants):\n";
                    for (const auto& c : catalog.getCourses()) {
                        if (c.participants.size() < 3) {
                            cout << c.course_name << ":\n";
                            for (const auto* s : c.participants) {
                                s->displayInfo();
                            }
                            cout << "Lecturer: ";
                            c.lecturer->displayInfo();
                        }
                    }
                    cout << "\nProgram ended.\n";
                    break;

                default:
                    cout << "Invalid option!\n";
            }
        } catch (const RegistrationException& e) {
            cout << e.what() << endl;
        }

    } while (choice != 5);

    
    for (Person* p : allPeople) {
        delete p;
    }

    return 0;
}