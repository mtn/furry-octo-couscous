import os
import webapp2
import jinja2
import re
import hashlib
import hmac

from string import letters
from google.appengine.ext import db

template_dir = os.path.join(os.path.dirname(__file__), 'templates')
jinja_env = jinja2.Environment(loader = jinja2.FileSystemLoader(template_dir),
                               autoescape = True)

def make_salt(length = 5):
    return ''.join(random.choice(letters) for x in xrange(length))

def make_pw_hash(password, salt = None):
    if not salt:
        salt = make_salt()
    h = hashlib.sha256(password + salt).hexdigest()
    return '%s,%s' % (salt, h)

def valid_pw(name, password, h):
    salt = h.split(',')[0]
    return h == make_pw_hash(name, password, salt)

class Handler(webapp2.RequestHandler):
    def write(self, *a, **kw):
        self.response.write(*a, **kw)

    def render_str(self, template, **params):
        t = jinja_env.get_template(template)
        return t.render(params)

    def render(self, template, **kw):
        self.write(self.render_str(template, **kw))

class Users(db.Model):
    username = db.StringProperty(required = True)
    passhash = db.StringProperty(required = True)
    email = db.StringProperty()

    @classmethod
    def register(cls, username, password, email):
        passhash = make_pw_hash(password)
        return Users(username=username,
                     passhash=passhash,
                     email = email)

    @classmethod
    def get_user_by_name(cls, username):
        a = Users.all().filter('username=',username)
        return a

    def login(self, user):
        self.set_secure_cookie('user_id', str(user.key().id()))

    def logout(self):
        self.response.headers.add_header('Set-Cookie', 'user_id=; Path=/')

class Signup(Handler):
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

        name_err, pass_err, match_err, email_err = "","","",""

        if not self.USER_RE.match(username):
            name_err = "That wasn't a valid username"
        if not self.PASS_RE.match(password):
            pass_err = "That wasn't a valid password"
        if (not pass_err) and password != verify:
            match_err = "Passwords didn't match"
        if email and (not self.MAIL_RE.match(email)):
            email_err = "That wasn't a valid email"

        if name_err or pass_err or match_err or email_err:
            self.render("signup.html",
                         username  = username,
                         email     = email,
                         name_err  = name_err,
                         pass_err  = pass_err,
                         match_err = match_err,
                         email_err = email_err)
        else:

            if a:
                self.render("signup.html", name_err = "That username already exists")
            else:
                b = Users.register(username, password, email)
                b.put()

                self.login(b)
                self.redirect('/')

class Main(Handler):
    def get(self):
        self.response.write("main page")

app = webapp2.WSGIApplication([('/', Main),
                               ('/signup', Signup)
                               # ('/welcome', Welcome)
                              ],
                              debug = True)
