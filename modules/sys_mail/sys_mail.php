<?php

/**
 * @copyright 2007-2021 Julian Bogdani
 * @license AGPL-3.0; see LICENSE
 * @since			Aug 28, 2012
 */

class sys_mail_ctrl extends Controller
{

    public function send()
    {
        $to = '';
        $subject = $this->post['subject'];
        $body = $this->post['body'] . "\n" . \tr::get('automatic_email_signature', [strtoupper($this->app)]);

        $headers = 'From: ' . $this->post['from']  . "\r\n" . 'Reply-To: ' . $this->post['from'] . "\r\n";

        $users_email = $this->db->query('SELECT * FROM ' . $this->prefix . 'users WHERE privilege IN (' . implode(', ', $this->post['to']) . ')');

        if ($users_email) {
            foreach ($users_email as $usr) {
                $res = mail($usr['email'], $subject, $body, $headers);

                $res ? $ok[] = $usr['name'] . ' <' . $usr['email'] . '>' : $no[] = $usr['name'] . ' <' . $usr['email'] . '>';
            }
        } else {
            echo '<div class="text-danger">'
                . '<strong>' . \tr::get('attention') . ':</strong> ' . \tr::get('no_user_with_selected_privilege') . '</p>'
                . '</div>';
            return;
        }

        if (count($ok) > 0) {
            $ret_text = '<p>' . \tr::get('ok_mail_sent_to_users', ['<ol><li>' . implode('</li><li>', $ok) . '</li></ol>']) . '</p>';
        }
        if (count($no) > 0) {
            $ret_text = '<p>' . \tr::get('error_mail_sent_to_users', ['<ol><li>' . implode('</li><li>', $no) . '</li></ol>']) . '</p>';
        }

        echo $ret_text;
    }


    public function showForm()
    {

        $this->render('sys_mail', 'form', [
            'adm_email' => $this->app . '_admin@bdus.cloud',
            'user_email' => $_SESSION['user']['email'],
            'privileges' => \utils::privilege('all', true)
        ]);
    }
}
