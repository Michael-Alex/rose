#ifndef __STL_TERM_HPP__
#define __STL_TERM_HPP__
#include <term.h++>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>

namespace term {

  class STLTerm;
  class STLAtom;
  class STLInt;
  class STLFloat;
  class STLList;
  class STLCompTerm;
  class STLVariable;

  enum TermVariant {
    Unknown, Atom, Int, Float, List, CompTerm, Variable;
  };

  /// Representation of a prolog term
  class STLTerm : virtual public Term {
  public:
    CompTerm* isCompTerm() { return variant == CompTerm ? static_cast<CompTerm*>(this) : NULL; }
    List* isList()         { return variant == List     ? static_cast<List*>(this)     : NULL; }
    Atom* isAtom()         { return variant == Atom     ? static_cast<Atom*>(this)     : NULL; }
    Int* isInt()           { return variant == Int      ? static_cast<Int*>(this)      : NULL; }
    Float* isFloat()       { return variant == Float    ? static_cast<Float*>(this)    : NULL; }
    Variable* isVariable() { return variant == Variable ? static_cast<Varible*>(this)  : NULL; }

    enum TermVariant variant;

    /// Properly quote and escape an atom if necessary
    static std::string quote(const std::string atom) {
      std::string s = escape(atom);
      if (atom.length() == 0) return "''";
      if (((atom.length() > 0) && (!islower(atom[0])) && (!isdigit(atom[0])))
	  || needs_quotes(atom)) {
	s = "'" + s + "'";
	return s;
      } else if (is_reserved_operator(atom)) {
	s = "(" + s + ")";
	return s;
      }
      return s;
    }

    // true if the pattern can be unified with the term
    bool matches(std::string pattern) { return false; assert(false && "not implemented"); }

  protected:

    static bool is_reserved_operator(const std::string s) {
      return s == "volatile";
    }

    static bool needs_quotes(const std::string s) {
      if (s.length() == 0) 
	return true;

      bool alldigits = isdigit(s[0]);
      for (std::string::const_iterator c = s.begin();
	   c != s.end(); ++c) {

	if (alldigits) {
	  if (!isdigit(*c))
	    return true;
	  else continue;
	}

	if (!islower(*c) && !isupper(*c) && 
	    !(*c == '_'))
	  return true;
      }
      return false;
    }

    // Escape non-printable characters
    static std::string escape(std::string s) {
      std::ostringstream r;
      for (unsigned int i = 0; i < s.length(); ++i) {
	unsigned char c = s[i];
	switch (c) {
	case '\\': r << "\\\\"; break; // Literal backslash
	case '\"': r << "\\\""; break; // Double quote
	case '\'': r << "\\'"; break;  // Single quote
	case '\n': r << "\\n"; break;  // Newline (line feed)
	case '\r': r << "\\r"; break;  // Carriage return
	case '\b': r << "\\b"; break;  // Backspace
	case '\t': r << "\\t"; break;  // Horizontal tab
	case '\f': r << "\\f"; break;  // Form feed
	case '\a': r << "\\a"; break;  // Alert (bell)
	case '\v': r << "\\v"; break;  // Vertical tab
	default:
	  if (c < 32 || c > 127) {
	    r << '\\' 
	      << std::oct 
	      << std::setfill('0') 
	      << std::setw(3) 
	      << (unsigned int)c // \nnn Character with octal value nnn
	      << '\\'; // Prolog expects this weird syntax with a trailing backslash
	  } else {
	    r << c;
	  }
	}
      }
      //cerr<<"escape("<<s<<") = "<< r <<endl;
      return r.str();
    }

  };



  ///class representing a prolog atom
  class STLAtom : virtual public Atom, virtual public STLTerm {
  public:
    ///constructor setting the string
    STLAtom(const std::string name = "#ERROR", bool escapedRepresentation = true)
      : variant(Atom), mName(name), mEscapedRepresentation(escapedRepresentation)
    { }
    /// the arity is always 0
    virtual int getArity() const {return 0;};
    /// an atom is always ground
    virtual bool isGround() const {return true;};
    ///return the string
    std::string getName() const {return mName;};
    /// return the string
    std::string getRepresentation() const {
      if (mEscapedRepresentation)
	return quote(mName);
      else
	// do not escape characters, but quote the whole string
	return "'" + mName + "'";
    }
  protected:
    /// the string
    std::string mName;
    /// flag indicating whether to quote the string when its representation is
    /// accessed -- in rare cases (preprocessing info) we do not want quoting
    bool mEscapedRepresentation;

    /// Properly quote and escape an atom if necessary
    static std::string quote(const std::string atom) {
      std::string s = escape(atom);
      if (atom.length() == 0) return "''";
      if (((atom.length() > 0) && (!islower(atom[0])) && (!isdigit(atom[0])))
	  || needs_quotes(atom)) {
	s = "'" + s + "'";
	return s;
      } else if (is_reserved_operator(atom)) {
	s = "(" + s + ")";
	return s;
      }
      return s;
    }

    static bool is_reserved_operator(const std::string s) {
      return s == "volatile";
    }

    static bool needs_quotes(const std::string s) {
      if (s.length() == 0) 
	return true;

      bool alldigits = isdigit(s[0]);
      for (std::string::const_iterator c = s.begin();
	   c != s.end(); ++c) {

	if (alldigits) {
	  if (!isdigit(*c))
	    return true;
	  else continue;
	}

	if (!islower(*c) && !isupper(*c) && 
	    !(*c == '_'))
	  return true;
      }
      return false;
    }

    // Escape non-printable characters
    static std::string escape(std::string s) {
      std::string r;
      for (unsigned int i = 0; i < s.length(); ++i) {
	unsigned char c = s[i];
	switch (c) {
	case '\\': r += "\\\\"; break; // Literal backslash
	case '\"': r += "\\\""; break; // Double quote
	case '\'': r += "\\'"; break;  // Single quote
	case '\n': r += "\\n"; break;  // Newline (line feed)
	case '\r': r += "\\r"; break;  // Carriage return
	case '\b': r += "\\b"; break;  // Backspace
	case '\t': r += "\\t"; break;  // Horizontal tab
	case '\f': r += "\\f"; break;  // Form feed
	case '\a': r += "\\a"; break;  // Alert (bell)
	case '\v': r += "\\v"; break;  // Vertical tab
	default:
	  if (c < 32 || c > 127) {
	    std::stringstream strm;
	    strm << '\\' 
		 << std::oct 
		 << std::setfill('0') 
		 << std::setw(3) 
		 << (unsigned int)c // \nnn Character with octal value nnn
		 << '\\'; // STL expects this weird syntax with a
	    // trailing backslash
	    r += strm.str();
	  } else {
	    r += c;
	  }
	}
      }
      //cerr<<"escape("<<s<<") = "<< r <<endl;
      return r;
    }

  };


  ///class representing a prolog integer
  class STLInt : public Int, public STLTerm {
  public:
    /// constructor sets the value
    STLInt(int value) : variant(Int), mValue(value) {}
    /// default constructor to make this constructible from templates
    STLInt() : mValue(0) {}
    /// return "Integer"
    virtual std::string getName() const {return "Integer";}

    /// return string representation of integer
    virtual std::string getRepresentation() const {
      std::ostringstream oss;
      oss << mValue;
      return oss.str();
    }

    ///the arity is 0
    virtual int getArity() const {return 0;}
    /// an integer is always ground
    virtual bool isGround() const {return true;}
    /// return value
    int64_t getValue() const {return mValue;}
  protected:
    /// the value
    int mValue;
  };

  ///class representing a prolog integer
  class STLFloat : public Float, public STLTerm {
  public:
    /// constructor sets the value
    STLFloat(double value) : variant(Float), mValue(value) {}

    ///the arity is 0
    int getArity() const {return 0;}
    /// an integer is always ground
    bool isGround() const {return true;}
    /// return "Float"
    std::string getName() const {return "Float";}
    /// return string representation of integer
    std::string getRepresentation() const {
      std::ostringstream oss;
      oss << std::fixed << mValue;
      return /*oss.str().find('.') != std::string::npos
	       ?*/ oss.str() /* 
				: oss.str()+".0"*/;
    }

    /// return value
    double getValue() const {return mValue;}
  protected:
    /// the value
    double mValue;
  };


  /// Representation of a compound prolog term.
  class STLCompTerm : public CompTerm, public STLTerm {
  public:
    /// Creates a compound term with the given name. no subterms added yet.
    STLCompTerm(const std::string name = "#ERROR") : variant(CompTerm), mName(name) {};

#if 0
    STLCompTerm(std::string name, size_t n, ...) : variant(CompTerm), mName(name) {
      if(n > 0) {
	va_list params;
	va_start(params, n);
	for (size_t i=0; i < n; i++) {
	  STLTerm* t = va_arg(params, STLTerm *);
	  assert(t != NULL);
	  addSubterm(t);
	}
	va_end(params);
      }
    }
#endif

    STLCompTerm(const std::string name, Term* t1)
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
    }

    STLCompTerm(const std::string name, Term* t1, Term* t2) 
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3) 
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3, 
		Term* t4) 
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3, 
		Term* t4, Term* t5) 
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
      mSubterms.push_back(t5);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3,
		Term* t4, Term* t5, Term* t6) 
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
      mSubterms.push_back(t5);
      mSubterms.push_back(t6);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3, 
		Term* t4, Term* t5, Term* t6, 
		Term* t7)
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
      mSubterms.push_back(t5);
      mSubterms.push_back(t6);
      mSubterms.push_back(t7);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3, 
		Term* t4, Term* t5, Term* t6, 
		Term* t7, Term* t8)
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
      mSubterms.push_back(t5);
      mSubterms.push_back(t6);
      mSubterms.push_back(t7);
      mSubterms.push_back(t8);
    }

    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3, 
		Term* t4, Term* t5, Term* t6, 
		Term* t7, Term* t8, Term* t9)
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
      mSubterms.push_back(t5);
      mSubterms.push_back(t6);
      mSubterms.push_back(t7);
      mSubterms.push_back(t8);
      mSubterms.push_back(t9);
    }
  
    STLCompTerm(const std::string name, 
		Term* t1, Term* t2, Term* t3, 
		Term* t4, Term* t5, Term* t6, 
		Term* t7, Term* t8, Term* t9,
		Term* t10)
      : variant(CompTerm), mName(name)
    {
      mSubterms.push_back(t1);
      mSubterms.push_back(t2);
      mSubterms.push_back(t3);
      mSubterms.push_back(t4);
      mSubterms.push_back(t5);
      mSubterms.push_back(t6);
      mSubterms.push_back(t7);
      mSubterms.push_back(t8);
      mSubterms.push_back(t9);
      mSubterms.push_back(t10);
    }

    int getArity() const { return mSubterms.size(); };
    bool isGround() const {
      std::vector<Term*>::const_iterator it;
      it = mSubterms.begin();
      while (it != mSubterms.end()) {
	if (!(*it)->isGround())
	  return false;
	it++;
      }
      return true;
    };

    /// Get the Functor
    std::string getName() const { return mName; };
    std::string getRepresentation() const {
      /*Pattern: name(...all subterms separated by commas..) */
      std::ostringstream rep;
      rep << quote(getName()) << "(";
      std::vector<Term*>::const_iterator it;
      it = mSubterms.begin();
      // append the representation of all subterms
      while (it != mSubterms.end()) {
	rep << (*it)->getRepresentation();
	// all but the last subterm are followed by a comma
	if(++it != mSubterms.end()) rep << ",";
      }
      rep << ")";
      return rep.str();
    };

    /// Get a vector of the subterms
    std::vector<Term *> getSubTerms() const { return mSubterms; };

    /// Add a subterm at the first position
    void addFirstSubTerm(Term* t) {mSubterms.insert(mSubterms.begin(),t);};

    /// Add a subterm at the last position
    void addSubterm(Term* t) {mSubterms.push_back(t);};

    /// the i-th subterm
    Term* at(int i) {return mSubterms.at(i);};
  protected:
    /// the name of the terms
    std::string mName;
    /// list of the subterms
    std::vector<Term *> mSubterms;
  };

  /// Representation of an infix operator.
  class STLInfixOperator : virtual public STLCompTerm {
  public:
    /// Creates a compound term with the given name. no subterms added yet.
    STLInfixOperator(std::string name) : STLCompTerm(name) {};
    std::string getRepresentation() const {
      assert(mSubterms.size() == 2);
      return mSubterms[0]->getRepresentation() + getName() + mSubterms[1]->getRepresentation();
    }
  };

  ///class representing a prolog variable
  class STLVariable : public STLTerm {
  public:
    ///constructor setting the name
    STLVariable(std::string name) : variant(CompTerm), mName(name) { 
      std::cerr<<"STLVariable("<<name<<" not implemented"<<std::endl;
      assert(false);
    };
    /// arity is always zero
    int getArity() const {return 0;};
    /// a variable isn't ground
    bool isGround() const {return false;};
    /// return the name
    std::string getName() const {return mName;};
    /// output the name
    std::string getRepresentation() const {return mName;};
    
  protected:
    /// the name
    std::string mName;
  };

  /// class representing a prolog list
  class STLList : public List, virtual public STLTerm {
  public:
    ///default constructor
    STLList() : variant(List) {}
    /// construct from vector
    STLList(std::deque<term::Term*>& v)  :mTerms(v) {}
    STLList(std::vector<term::Term*>& v) :mTerms(v.begin(), v.end()) {}
  
    /// return size of the list
    int getArity() const {return mTerms.size();}
    /// are all the members ground?
    bool isGround() const {
      //List is ground if all elements are.
      std::deque<Term*>::const_iterator it = mTerms.begin();
      while(it != mTerms.end()) {
	if (!(*it)->isGround())
	  return false;
	it++;
      }
      return true;
    }

    /// the predicate name of a list in prolog is .
    std::string getName() const {return ".";}
    /// output the representation
    std::string getRepresentation() const {
      /*Pattern: name(...all subterms separated by commas..) */
      std::ostringstream rep;
      rep << "[";
      std::deque<Term*>::const_iterator it;
      it = mTerms.begin();
      // append the representation of all subterms
      while (it != mTerms.end()) {
	rep << (*it)->getRepresentation();
	// all but the last subterm are followed by a comma
	if(++it != mTerms.end()) rep << ",";
      }
      rep << "]";
      return rep.str();
    }

    /// add a list element
    void addElement(term::Term* t) {mTerms.push_back(t);}

    /// add a list element at the beginning
    void addFirstElement(term::Term* t) {mTerms.push_front(t);}

    /// get the i-th element
    Term* at(int i) {return mTerms.at(i);}

    std::deque<Term*>* getSuccs() { return &mTerms; };

  protected:
    /// the successors
    std::deque<term::Term*> mTerms;
  };


  class STLTermFactory : public TermFactory {
    /// create a new atom
    Atom* makeAtom(const std::string& name, bool escape) const 
    { return new STLAtom(name, escape); };

    /// create a new int
    Int* makeInt(const int value) const { return new STLInt(value); }

    /// create a new float
    Float* makeFloat(const float value) const { return new STLFloat(value); }

    /// create a new List
    List* makeList() const { return new STLList(); }
    List* makeList(std::deque<term::Term*>& v) const { return new STLList(v); }
    List* makeList(std::vector<term::Term*>& v) const { return new STLList(v); }

    /// create a new compound term
    //  Yes, I do know about variadic functions.
    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1) const
    { return new STLCompTerm(name, t1); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2) const
    { return new STLCompTerm(name, t1, t2); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3) const
    { return new STLCompTerm(name, t1, t2, t3); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4) const
    { return new STLCompTerm(name, t1, t2, t3, t4); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4, term::Term* t5) const
    { return new STLCompTerm(name, t1, t2, t3, t4, t5); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4, term::Term* t5, term::Term* t6) const
    { return new STLCompTerm(name, t1, t2, t3, t4, t5, t6); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4, term::Term* t5, term::Term* t6,
			   term::Term* t7) const
    { return new STLCompTerm(name, t1, t2, t3, t4, t5, t6, t7); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4, term::Term* t5, term::Term* t6,
			   term::Term* t7, term::Term* t8) const
    { return new STLCompTerm(name, t1, t2, t3, t4, t5, t6, t7, t8); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4, term::Term* t5, term::Term* t6,
			   term::Term* t7, term::Term* t8, term::Term* t9) const
    { return new STLCompTerm(name, t1, t2, t3, t4, t5, t6, t7, t8, t9); }

    CompTerm* makeCompTerm(const std::string& name, 
			   term::Term* t1, term::Term* t2, term::Term* t3,
			   term::Term* t4, term::Term* t5, term::Term* t6,
			   term::Term* t7, term::Term* t8, term::Term* t9,
			   term::Term* t10) const
    { return new STLCompTerm(name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10); }
  };


}
#endif
