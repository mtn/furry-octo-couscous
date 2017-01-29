import os
import webapp2
import jinja2
import cgi
import re

template_dir = os.path.join(os.path.dirname(__file__),'templates')
jinja_env = jinja2.Environment(loader = jinja2.FileSystemLoader(template_dir),
                               autoescape=True)

class Handler(webapp2.RequestHandler):
    def write(self, *a, **kw):
        self.response.write(*a,**kw)

    def render_str(self, template, **params):
        t = jinja_env.get_template(template)
        return t.render(params)

    def render(self, template, **kw):
        self.write(self.render_str(template, **kw))

class MainPage(Handler):
    def get(self):
        self.response.write("/rot13 for rot13\n /user-signup for user signup")

class Rot13(Handler):
    def get(self):
        self.render("rot13.html")
    def post(self):
        to_shift = self.request.get('to_shift')

        self.render("rot13.html",text = self.rot13(to_shift))

    @staticmethod
    def rot13(to_shift):
        shifted = ""
        for c in to_shift:
            if c.isalpha():
                if c.isupper():
                    shifted += chr(((ord(c) - ord('A')  + 13) % 26) + ord('A'))
                else:
                    shifted += chr(((ord(c) - ord('a')  + 13) % 26) + ord('a'))
            else:
                shifted += cgi.escape(c)

        return shifted

class UserSignup(Handler):
    USER_RE = re.compile(r"^[a-zA-Z0-9_-]{3,20}$")
    PASS_RE = re.compile(r"^.{3,20}$")
    MAIL_RE = re.compile(r"^[\S]+@[\S]+.[\S]+$")

    def get(self):
        self.render("signup.html")

    def post(self):
        username = self.request.get('username')
        password = self.request.get('password')
        verify = self.request.get('verify')
        email = self.request.get('email')

        name_err, pass_err, email_err, match_err = "","","",""

        if not self.USER_RE.match(username):
            name_err = "That wasn't a valid username."

        if not self.PASS_RE.match(password):
            pass_err = "That wasn't a valid password."

        if (not pass_err) and password != verify:
            match_err = "Your passwords didn't match."

        if email and not self.MAIL_RE.match(email):
            email_err = "That wasn't a valid email."

        # self.response.write(name_err + pass_err + match_err + email_err)
        if name_err or pass_err or match_err or email_err:
            self.render("signup.html",
                        username  = username,
                        password  = password,
                        verify    = verify,
                        email     = email,
                        name_err  = name_err,
                        pass_err  = pass_err,
                        match_err = match_err,
                        email_err = email_err)
        else:
            self.redirect('/welcome?username=' + username)

class Welcome(Handler):
    def get(self):
        self.response.write("Thanks, " + self.request.get('username') + "!")

app = webapp2.WSGIApplication([('/', MainPage),
                               ('/rot13', Rot13),
                               ('/user-signup', UserSignup),
                               ('/welcome', Welcome)
                              ],
                              debug=True)
